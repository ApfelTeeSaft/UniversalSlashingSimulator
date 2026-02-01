/**
 * UniversalSlashingSimulator - Building Manager Implementation
 */

#include "BuildingManager.h"
#include "../Inventory/InventoryManager.h"
#include "../../Core/Logging/Log.h"
#include "../../Engine/EngineCore.h"

namespace USS
{
    static FBuildingManager g_LocalBuildingManager;

    FBuildingManager& GetLocalBuildingManager()
    {
        return g_LocalBuildingManager;
    }

    FBuildingManager::FBuildingManager()
    {
        m_BuildPreview = FBuildPreview();
    }

    FBuildingManager::~FBuildingManager()
    {
        Shutdown();
    }

    EResult FBuildingManager::Initialize(FInventoryManager* InventoryManager)
    {
        USS_LOG("Initializing Building Manager...");

        m_pInventoryManager = InventoryManager;

        m_Buildings.clear();
        m_Traps.clear();
        m_GridToBuildingId.clear();
        m_EventCallbacks.clear();

        m_bIsInBuildMode = false;
        m_bIsPlacingTrap = false;
        m_CurrentBuildType = EBuildingType::None;
        m_CurrentMaterial = EBuildingMaterial::Wood;

        // Find building manager actors in world
        // TODO: Locate AFortBuildingManager

        USS_LOG("Building Manager initialized");
        return EResult::Success;
    }

    void FBuildingManager::Shutdown()
    {
        ExitBuildMode();
        ExitTrapPlacementMode();

        m_Buildings.clear();
        m_Traps.clear();
        m_GridToBuildingId.clear();
        m_EventCallbacks.clear();

        m_pInventoryManager = nullptr;
        m_BuildingManagerActor = UObjectWrapper();
        m_TrapManagerActor = UObjectWrapper();
    }

    void FBuildingManager::Update()
    {
        // Update building construction progress
        for (auto& Pair : m_Buildings)
        {
            FBuildingPiece& Building = Pair.second;

            if (Building.State == EBuildingState::Building)
            {
                // Progress construction
                // TODO: Check actual build progress from engine
            }
            else if (Building.State == EBuildingState::Repairing)
            {
                // Progress repair
                float DeltaTime = 1.0f / 30.0f;  // Placeholder
                Building.Stats.CurrentHealth += Building.Stats.RepairRate * DeltaTime;

                if (Building.Stats.CurrentHealth >= Building.Stats.MaxHealth)
                {
                    Building.Stats.CurrentHealth = Building.Stats.MaxHealth;
                    Building.State = EBuildingState::Built;
                }
            }
        }

        // Update traps
        float DeltaTime = 1.0f / 30.0f;  // Placeholder
        UpdateTraps(DeltaTime);
    }

    EResult FBuildingManager::EnterBuildMode(EBuildingType Type)
    {
        if (Type == EBuildingType::None || Type == EBuildingType::Trap)
        {
            return EResult::InvalidParameter;
        }

        m_bIsInBuildMode = true;
        m_CurrentBuildType = Type;

        // Initialize preview
        m_BuildPreview = FBuildPreview();
        m_BuildPreview.Type = Type;
        m_BuildPreview.Material = m_CurrentMaterial;
        m_BuildPreview.Cost = GetDefaultBuildCost(m_CurrentMaterial);

        USS_LOG("Entered build mode: Type=%d, Material=%d",
            static_cast<int>(Type), static_cast<int>(m_CurrentMaterial));

        return EResult::Success;
    }

    void FBuildingManager::ExitBuildMode()
    {
        if (m_bIsInBuildMode)
        {
            m_bIsInBuildMode = false;
            m_CurrentBuildType = EBuildingType::None;
            m_BuildPreview = FBuildPreview();

            USS_LOG("Exited build mode");
        }
    }

    void FBuildingManager::SetBuildMaterial(EBuildingMaterial Material)
    {
        m_CurrentMaterial = Material;

        if (m_bIsInBuildMode)
        {
            m_BuildPreview.Material = Material;
            m_BuildPreview.Cost = GetDefaultBuildCost(Material);
        }

        USS_LOG("Set build material: %d", static_cast<int>(Material));
    }

    void FBuildingManager::CycleMaterial()
    {
        switch (m_CurrentMaterial)
        {
        case EBuildingMaterial::Wood:
            SetBuildMaterial(EBuildingMaterial::Stone);
            break;
        case EBuildingMaterial::Stone:
            SetBuildMaterial(EBuildingMaterial::Metal);
            break;
        case EBuildingMaterial::Metal:
        default:
            SetBuildMaterial(EBuildingMaterial::Wood);
            break;
        }
    }

    void FBuildingManager::UpdateBuildPreview(float LocationX, float LocationY, float LocationZ, float Rotation)
    {
        if (!m_bIsInBuildMode)
            return;

        m_BuildPreview.LocationX = LocationX;
        m_BuildPreview.LocationY = LocationY;
        m_BuildPreview.LocationZ = LocationZ;
        m_BuildPreview.Rotation = Rotation;

        // Validate placement
        m_BuildPreview.bIsOverlapping = CheckOverlap(LocationX, LocationY, LocationZ);
        m_BuildPreview.bIsFloating = !CheckSupport(LocationX, LocationY, LocationZ, m_BuildPreview.Type);

        m_BuildPreview.bIsValidPlacement = !m_BuildPreview.bIsOverlapping && !m_BuildPreview.bIsFloating;

        // Check resources
        if (m_pInventoryManager)
        {
            int32 Wood = m_pInventoryManager->GetWoodCount();
            int32 Stone = m_pInventoryManager->GetStoneCount();
            int32 Metal = m_pInventoryManager->GetMetalCount();

            m_BuildPreview.bCanAfford = m_BuildPreview.Cost.CanAfford(Wood, Stone, Metal);
        }
    }

    EResult FBuildingManager::ConfirmBuild()
    {
        if (!m_bIsInBuildMode)
        {
            return EResult::InvalidState;
        }

        if (!m_BuildPreview.bIsValidPlacement)
        {
            USS_WARN("Cannot build: Invalid placement");
            return EResult::InvalidPlacement;
        }

        if (!m_BuildPreview.bCanAfford)
        {
            USS_WARN("Cannot build: Insufficient resources");
            return EResult::InsufficientResources;
        }

        if (IsAtBuildLimit())
        {
            USS_WARN("Cannot build: At build limit");
            return EResult::BuildLimitReached;
        }

        // Consume resources
        if (m_pInventoryManager)
        {
            const FBuildingCost& Cost = m_BuildPreview.Cost;

            if (Cost.WoodCost > 0)
                m_pInventoryManager->ConsumeResources(EResourceType::Wood, Cost.WoodCost);
            if (Cost.StoneCost > 0)
                m_pInventoryManager->ConsumeResources(EResourceType::Stone, Cost.StoneCost);
            if (Cost.MetalCost > 0)
                m_pInventoryManager->ConsumeResources(EResourceType::Metal, Cost.MetalCost);
        }

        // Create building
        FBuildingPiece NewBuilding;
        NewBuilding.BuildingId = GenerateBuildingId();
        NewBuilding.Type = m_BuildPreview.Type;
        NewBuilding.Material = m_BuildPreview.Material;
        NewBuilding.Tier = EBuildingTier::Tier1;
        NewBuilding.State = EBuildingState::Building;
        NewBuilding.Stats = GetDefaultBuildStats(m_BuildPreview.Material);
        NewBuilding.Cost = m_BuildPreview.Cost;
        NewBuilding.bIsPlayerBuilt = true;

        // Apply build speed multiplier
        NewBuilding.Stats.BuildTime /= m_BuildSpeedMultiplier;

        // Grid position (TODO: implement proper grid snapping)
        NewBuilding.GridX = static_cast<int32>(m_BuildPreview.LocationX / 512.0f);
        NewBuilding.GridY = static_cast<int32>(m_BuildPreview.LocationY / 512.0f);
        NewBuilding.GridZ = static_cast<int32>(m_BuildPreview.LocationZ / 512.0f);
        NewBuilding.Rotation = m_BuildPreview.Rotation;

        // Store building
        m_Buildings[NewBuilding.BuildingId] = NewBuilding;

        // Register in grid lookup
        uint64 GridKey = (static_cast<uint64>(NewBuilding.GridX) << 40) |
                        (static_cast<uint64>(NewBuilding.GridY) << 20) |
                        static_cast<uint64>(NewBuilding.GridZ);
        m_GridToBuildingId[GridKey] = NewBuilding.BuildingId;

        // Notify
        FBuildingChangeEvent Event;
        Event.Type = FBuildingChangeEvent::EChangeType::Built;
        Event.BuildingId = NewBuilding.BuildingId;
        Event.NewHealth = NewBuilding.Stats.CurrentHealth;
        NotifyChange(Event);

        USS_LOG("Built %d at grid (%d, %d, %d) - ID: %s",
            static_cast<int>(NewBuilding.Type),
            NewBuilding.GridX, NewBuilding.GridY, NewBuilding.GridZ,
            NewBuilding.BuildingId.c_str());

        return EResult::Success;
    }

    void FBuildingManager::CancelBuild()
    {
        m_BuildPreview = FBuildPreview();
        m_BuildPreview.Type = m_CurrentBuildType;
        m_BuildPreview.Material = m_CurrentMaterial;
    }

    const FBuildingPiece* FBuildingManager::GetBuilding(const std::string& BuildingId) const
    {
        auto It = m_Buildings.find(BuildingId);
        return (It != m_Buildings.end()) ? &It->second : nullptr;
    }

    const FBuildingPiece* FBuildingManager::GetBuildingAtGrid(int32 X, int32 Y, int32 Z) const
    {
        uint64 GridKey = (static_cast<uint64>(X) << 40) |
                        (static_cast<uint64>(Y) << 20) |
                        static_cast<uint64>(Z);

        auto It = m_GridToBuildingId.find(GridKey);
        if (It != m_GridToBuildingId.end())
        {
            return GetBuilding(It->second);
        }
        return nullptr;
    }

    std::vector<const FBuildingPiece*> FBuildingManager::GetPlayerBuildings(const std::string& PlayerId) const
    {
        std::vector<const FBuildingPiece*> Result;

        for (const auto& Pair : m_Buildings)
        {
            if (Pair.second.OwnerId == PlayerId || Pair.second.bIsPlayerBuilt)
            {
                Result.push_back(&Pair.second);
            }
        }

        return Result;
    }

    int32 FBuildingManager::GetPlayerBuildingCount(const std::string& PlayerId) const
    {
        int32 Count = 0;

        for (const auto& Pair : m_Buildings)
        {
            if (Pair.second.OwnerId == PlayerId || Pair.second.bIsPlayerBuilt)
            {
                ++Count;
            }
        }

        return Count;
    }

    EResult FBuildingManager::DamageBuilding(const std::string& BuildingId, float Damage, void* DamageCauser)
    {
        auto It = m_Buildings.find(BuildingId);
        if (It == m_Buildings.end())
        {
            return EResult::BuildingNotFound;
        }

        FBuildingPiece& Building = It->second;
        float OldHealth = Building.Stats.CurrentHealth;

        // Apply damage resistance
        float ActualDamage = Damage * (1.0f - Building.Stats.DamageResistance);
        Building.Stats.CurrentHealth -= ActualDamage;

        FBuildingChangeEvent Event;
        Event.BuildingId = BuildingId;
        Event.OldHealth = OldHealth;
        Event.NewHealth = Building.Stats.CurrentHealth;
        Event.Damage = ActualDamage;
        Event.DamageCauser = DamageCauser;

        if (Building.Stats.CurrentHealth <= 0.0f)
        {
            Building.Stats.CurrentHealth = 0.0f;
            Building.State = EBuildingState::Destroying;

            Event.Type = FBuildingChangeEvent::EChangeType::Destroyed;

            // Remove from grid
            uint64 GridKey = (static_cast<uint64>(Building.GridX) << 40) |
                            (static_cast<uint64>(Building.GridY) << 20) |
                            static_cast<uint64>(Building.GridZ);
            m_GridToBuildingId.erase(GridKey);

            // Remove traps attached to this building
            std::vector<std::string> TrapsToRemove;
            for (const auto& TrapPair : m_Traps)
            {
                if (TrapPair.second.AttachedBuildingId == BuildingId)
                {
                    TrapsToRemove.push_back(TrapPair.first);
                }
            }
            for (const auto& TrapId : TrapsToRemove)
            {
                m_Traps.erase(TrapId);
            }

            m_Buildings.erase(It);

            USS_LOG("Building destroyed: %s", BuildingId.c_str());
        }
        else
        {
            Building.State = EBuildingState::Damaged;
            Event.Type = FBuildingChangeEvent::EChangeType::Damaged;
        }

        NotifyChange(Event);

        return EResult::Success;
    }

    EResult FBuildingManager::RepairBuilding(const std::string& BuildingId, float Amount)
    {
        auto It = m_Buildings.find(BuildingId);
        if (It == m_Buildings.end())
        {
            return EResult::BuildingNotFound;
        }

        FBuildingPiece& Building = It->second;

        if (Building.IsFullHealth())
        {
            return EResult::Success;  // Already full
        }

        float OldHealth = Building.Stats.CurrentHealth;
        Building.Stats.CurrentHealth += Amount;

        if (Building.Stats.CurrentHealth >= Building.Stats.MaxHealth)
        {
            Building.Stats.CurrentHealth = Building.Stats.MaxHealth;
            Building.State = EBuildingState::Built;
        }
        else
        {
            Building.State = EBuildingState::Repairing;
        }

        FBuildingChangeEvent Event;
        Event.Type = FBuildingChangeEvent::EChangeType::Repaired;
        Event.BuildingId = BuildingId;
        Event.OldHealth = OldHealth;
        Event.NewHealth = Building.Stats.CurrentHealth;
        NotifyChange(Event);

        return EResult::Success;
    }

    EResult FBuildingManager::UpgradeBuilding(const std::string& BuildingId)
    {
        auto It = m_Buildings.find(BuildingId);
        if (It == m_Buildings.end())
        {
            return EResult::BuildingNotFound;
        }

        FBuildingPiece& Building = It->second;

        if (Building.Tier >= EBuildingTier::Tier3)
        {
            USS_WARN("Building already at max tier");
            return EResult::InvalidState;
        }

        // Check upgrade cost (STW uses different resources for upgrades)
        // TODO: Consume upgrade materials

        // Upgrade tier
        Building.Tier = static_cast<EBuildingTier>(static_cast<int>(Building.Tier) + 1);
        Building.State = EBuildingState::Upgrading;

        // Increase stats
        Building.Stats.MaxHealth *= 1.5f;
        Building.Stats.CurrentHealth = Building.Stats.MaxHealth;
        Building.Stats.DamageResistance += 0.05f;

        FBuildingChangeEvent Event;
        Event.Type = FBuildingChangeEvent::EChangeType::Upgraded;
        Event.BuildingId = BuildingId;
        Event.NewHealth = Building.Stats.CurrentHealth;
        NotifyChange(Event);

        USS_LOG("Upgraded building %s to tier %d", BuildingId.c_str(), static_cast<int>(Building.Tier));

        Building.State = EBuildingState::Built;

        return EResult::Success;
    }

    EResult FBuildingManager::DemolishBuilding(const std::string& BuildingId)
    {
        auto It = m_Buildings.find(BuildingId);
        if (It == m_Buildings.end())
        {
            return EResult::BuildingNotFound;
        }

        const FBuildingPiece& Building = It->second;

        // Refund partial resources (50%)
        if (m_pInventoryManager)
        {
            FInventoryItem Refund;
            Refund.Category = EItemCategory::Resource;

            if (Building.Cost.WoodCost > 0)
            {
                Refund.TemplateId = "Resource:Wood";
                Refund.Count = Building.Cost.WoodCost / 2;
                m_pInventoryManager->AddItem(Refund);
            }
            if (Building.Cost.StoneCost > 0)
            {
                Refund.TemplateId = "Resource:Stone";
                Refund.Count = Building.Cost.StoneCost / 2;
                m_pInventoryManager->AddItem(Refund);
            }
            if (Building.Cost.MetalCost > 0)
            {
                Refund.TemplateId = "Resource:Metal";
                Refund.Count = Building.Cost.MetalCost / 2;
                m_pInventoryManager->AddItem(Refund);
            }
        }

        // Remove from grid
        uint64 GridKey = (static_cast<uint64>(Building.GridX) << 40) |
                        (static_cast<uint64>(Building.GridY) << 20) |
                        static_cast<uint64>(Building.GridZ);
        m_GridToBuildingId.erase(GridKey);

        // Remove traps
        std::vector<std::string> TrapsToRemove;
        for (const auto& TrapPair : m_Traps)
        {
            if (TrapPair.second.AttachedBuildingId == BuildingId)
            {
                TrapsToRemove.push_back(TrapPair.first);
            }
        }
        for (const auto& TrapId : TrapsToRemove)
        {
            m_Traps.erase(TrapId);
        }

        m_Buildings.erase(It);

        FBuildingChangeEvent Event;
        Event.Type = FBuildingChangeEvent::EChangeType::Destroyed;
        Event.BuildingId = BuildingId;
        NotifyChange(Event);

        USS_LOG("Demolished building: %s", BuildingId.c_str());

        return EResult::Success;
    }

    EResult FBuildingManager::EditBuilding(const std::string& BuildingId, EBuildingType NewType)
    {
        auto It = m_Buildings.find(BuildingId);
        if (It == m_Buildings.end())
        {
            return EResult::BuildingNotFound;
        }

        // Can only edit walls/floors/etc
        FBuildingPiece& Building = It->second;

        if (Building.Type == EBuildingType::Trap ||
            Building.Type == EBuildingType::DefenseBuild ||
            Building.Type == EBuildingType::StructuralBuild)
        {
            return EResult::InvalidParameter;
        }

        Building.Type = NewType;

        USS_LOG("Edited building %s to type %d", BuildingId.c_str(), static_cast<int>(NewType));

        return EResult::Success;
    }

    EResult FBuildingManager::EnterTrapPlacementMode(ETrapType TrapType, const std::string& TrapItemId)
    {
        if (TrapType == ETrapType::None)
        {
            return EResult::InvalidParameter;
        }

        m_bIsPlacingTrap = true;
        m_CurrentTrapType = TrapType;
        m_CurrentTrapItemId = TrapItemId;

        USS_LOG("Entered trap placement mode: Type=%d", static_cast<int>(TrapType));

        return EResult::Success;
    }

    void FBuildingManager::ExitTrapPlacementMode()
    {
        if (m_bIsPlacingTrap)
        {
            m_bIsPlacingTrap = false;
            m_CurrentTrapType = ETrapType::None;
            m_CurrentTrapItemId.clear();

            USS_LOG("Exited trap placement mode");
        }
    }

    EResult FBuildingManager::ConfirmTrapPlacement(const std::string& AttachedBuildingId)
    {
        if (!m_bIsPlacingTrap)
        {
            return EResult::InvalidState;
        }

        // Verify building exists
        const FBuildingPiece* Building = GetBuilding(AttachedBuildingId);
        if (!Building)
        {
            return EResult::BuildingNotFound;
        }

        // Create trap
        FTrapInstance NewTrap;
        NewTrap.TrapId = GenerateTrapId();
        NewTrap.Type = m_CurrentTrapType;
        NewTrap.State = EBuildingState::Built;
        NewTrap.AttachedBuildingId = AttachedBuildingId;
        NewTrap.GridX = Building->GridX;
        NewTrap.GridY = Building->GridY;
        NewTrap.GridZ = Building->GridZ;
        NewTrap.bIsArmed = true;

        // Set default trap stats
        NewTrap.Stats.Damage = 50.0f * m_TrapDamageMultiplier;
        NewTrap.Stats.ReloadTime = 2.0f;
        NewTrap.Stats.MaxDurability = 30;
        NewTrap.Stats.CurrentDurability = 30;

        // Consume trap item from inventory
        if (m_pInventoryManager && !m_CurrentTrapItemId.empty())
        {
            m_pInventoryManager->RemoveItem(m_CurrentTrapItemId, 1);
        }

        m_Traps[NewTrap.TrapId] = NewTrap;

        FBuildingChangeEvent Event;
        Event.Type = FBuildingChangeEvent::EChangeType::TrapPlaced;
        Event.BuildingId = NewTrap.TrapId;
        NotifyChange(Event);

        USS_LOG("Placed trap %s on building %s", NewTrap.TrapId.c_str(), AttachedBuildingId.c_str());

        return EResult::Success;
    }

    const FTrapInstance* FBuildingManager::GetTrap(const std::string& TrapId) const
    {
        auto It = m_Traps.find(TrapId);
        return (It != m_Traps.end()) ? &It->second : nullptr;
    }

    std::vector<const FTrapInstance*> FBuildingManager::GetTrapsOnBuilding(const std::string& BuildingId) const
    {
        std::vector<const FTrapInstance*> Result;

        for (const auto& Pair : m_Traps)
        {
            if (Pair.second.AttachedBuildingId == BuildingId)
            {
                Result.push_back(&Pair.second);
            }
        }

        return Result;
    }

    EResult FBuildingManager::TriggerTrap(const std::string& TrapId)
    {
        auto It = m_Traps.find(TrapId);
        if (It == m_Traps.end())
        {
            return EResult::TrapNotFound;
        }

        FTrapInstance& Trap = It->second;

        if (!Trap.IsReady())
        {
            return EResult::TrapNotReady;
        }

        ProcessTrapTrigger(Trap);

        return EResult::Success;
    }

    EResult FBuildingManager::ReloadTrap(const std::string& TrapId)
    {
        auto It = m_Traps.find(TrapId);
        if (It == m_Traps.end())
        {
            return EResult::TrapNotFound;
        }

        FTrapInstance& Trap = It->second;
        Trap.CooldownRemaining = 0.0f;
        Trap.bIsArmed = true;

        USS_LOG("Reloaded trap: %s", TrapId.c_str());

        return EResult::Success;
    }

    bool FBuildingManager::IsAtBuildLimit() const
    {
        return static_cast<int32>(m_Buildings.size()) >= m_BuildLimit;
    }

    void FBuildingManager::ApplyConstructorPerks(float BuildSpeed, float TrapDamage, int32 ExtraBuildLimit)
    {
        m_BuildSpeedMultiplier = BuildSpeed;
        m_TrapDamageMultiplier = TrapDamage;
        m_BuildLimit += ExtraBuildLimit;

        USS_LOG("Applied constructor perks: BuildSpeed=%.2f, TrapDamage=%.2f, ExtraLimit=%d",
            BuildSpeed, TrapDamage, ExtraBuildLimit);
    }

    void FBuildingManager::RegisterEventCallback(FBuildingEventCallback Callback)
    {
        if (Callback)
        {
            m_EventCallbacks.push_back(std::move(Callback));
        }
    }

    void FBuildingManager::OnProcessEvent(void* Object, void* Function, void* Params)
    {
        // Handle building-related ProcessEvents
    }

    std::string FBuildingManager::GenerateBuildingId() const
    {
        char Buffer[64];
        snprintf(Buffer, sizeof(Buffer), "bld_%u", ++m_BuildingIdCounter);
        return Buffer;
    }

    std::string FBuildingManager::GenerateTrapId() const
    {
        char Buffer[64];
        snprintf(Buffer, sizeof(Buffer), "trap_%u", ++m_TrapIdCounter);
        return Buffer;
    }

    bool FBuildingManager::ValidatePlacement(const FBuildPreview& Preview) const
    {
        return Preview.bIsValidPlacement && Preview.bCanAfford;
    }

    bool FBuildingManager::CheckOverlap(float X, float Y, float Z) const
    {
        int32 GridX = static_cast<int32>(X / 512.0f);
        int32 GridY = static_cast<int32>(Y / 512.0f);
        int32 GridZ = static_cast<int32>(Z / 512.0f);

        return GetBuildingAtGrid(GridX, GridY, GridZ) != nullptr;
    }

    bool FBuildingManager::CheckSupport(float X, float Y, float Z, EBuildingType Type) const
    {
        // Floors can be placed on ground or supported by walls
        // Walls need floor below or adjacent wall
        // Ramps need floor or ground

        // Simplified: Always allow for now
        // TODO: Would check adjacent buildings and terrain

        return true;
    }

    void FBuildingManager::NotifyChange(const FBuildingChangeEvent& Event)
    {
        for (const auto& Callback : m_EventCallbacks)
        {
            if (Callback)
            {
                Callback(Event);
            }
        }
    }

    void FBuildingManager::UpdateTraps(float DeltaTime)
    {
        for (auto& Pair : m_Traps)
        {
            FTrapInstance& Trap = Pair.second;

            // Update cooldown
            if (Trap.CooldownRemaining > 0.0f)
            {
                Trap.CooldownRemaining -= DeltaTime;
                if (Trap.CooldownRemaining < 0.0f)
                {
                    Trap.CooldownRemaining = 0.0f;
                }
            }

            // TODO: Check for enemies in range and trigger
        }
    }

    void FBuildingManager::ProcessTrapTrigger(FTrapInstance& Trap)
    {
        Trap.bIsTriggered = true;
        Trap.Stats.CurrentDurability -= Trap.Stats.UsesPerActivation;
        Trap.CooldownRemaining = Trap.Stats.ReloadTime;

        FBuildingChangeEvent Event;
        Event.Type = FBuildingChangeEvent::EChangeType::TrapTriggered;
        Event.BuildingId = Trap.TrapId;
        NotifyChange(Event);

        USS_LOG("Trap triggered: %s (durability: %d/%d)",
            Trap.TrapId.c_str(),
            Trap.Stats.CurrentDurability,
            Trap.Stats.MaxDurability);

        if (Trap.Stats.CurrentDurability <= 0)
        {
            Trap.bIsArmed = false;
            Trap.State = EBuildingState::Destroying;

            FBuildingChangeEvent DestroyEvent;
            DestroyEvent.Type = FBuildingChangeEvent::EChangeType::TrapDestroyed;
            DestroyEvent.BuildingId = Trap.TrapId;
            NotifyChange(DestroyEvent);

            USS_LOG("Trap exhausted: %s", Trap.TrapId.c_str());
        }

        Trap.bIsTriggered = false;
    }

}