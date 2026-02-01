/**
 * UniversalSlashingSimulator - STW Game Mode Implementation
 *
 * Based on farmstead_plate.cpp from PolarisV2-STW.
 */

#include "STWGameMode.h"
#include "../../Core/Logging/Log.h"
#include "../../Core/Hooks/HookTypes.h"
#include "../../Engine/EngineCore.h"
#include "../Missions/MissionManager.h"
#include "../Inventory/InventoryManager.h"
#include "../Building/BuildingManager.h"
#include "../Player/STWPlayerController.h"
#include <cstring>

namespace USS
{
    FSTWGameMode::FSTWGameMode()
        : m_State(ESTWGameState::None)
        , m_bWorldReady(false)
        , m_bPlayersLoaded(false)
    {
    }

    FSTWGameMode::~FSTWGameMode()
    {
        Shutdown();
    }

    FSTWGameMode& FSTWGameMode::Get()
    {
        static FSTWGameMode Instance;
        return Instance;
    }

    EResult FSTWGameMode::Initialize(const FSTWGameConfig& Config)
    {
        if (m_State != ESTWGameState::None)
            return EResult::AlreadyInitialized;

        USS_LOG("Initializing STW GameMode...");
        USS_LOG("  Zone: %s", Config.ZoneName.c_str());
        USS_LOG("  Mission: %s", Config.MissionBlueprint.c_str());

        m_Config = Config;
        SetState(ESTWGameState::Initializing);

        // Create managers
        m_pMissionManager = std::make_unique<FMissionManager>();
        m_pInventoryManager = std::make_unique<FInventoryManager>();
        m_pBuildingManager = std::make_unique<FBuildingManager>();

        // Register for ProcessEvent callbacks (@timmie implements hooks)
        // When hooks are implemented, register a callback that forwards to OnProcessEvent:
        // Hook::GetProcessEventDispatcher().RegisterPre([this](void* Obj, void* Func, void* Params) -> bool {
        //     OnProcessEvent(Obj, Func, Params);
        //     return true;
        // });

        // Initialize managers
        if (m_pMissionManager->Initialize() != EResult::Success)
        {
            USS_WARN("Mission manager initialization incomplete");
        }

        if (m_pInventoryManager->Initialize(nullptr) != EResult::Success)
        {
            USS_WARN("Inventory manager initialization incomplete");
        }

        if (m_pBuildingManager->Initialize(m_pInventoryManager.get()) != EResult::Success)
        {
            USS_WARN("Building manager initialization incomplete");
        }

        SetState(ESTWGameState::WaitingForWorld);

        USS_LOG("STW GameMode initialized, waiting for world...");
        return EResult::Success;
    }

    void FSTWGameMode::Shutdown()
    {
        if (m_State == ESTWGameState::None || m_State == ESTWGameState::Shutdown)
            return;

        USS_LOG("Shutting down STW GameMode...");

        SetState(ESTWGameState::Shutdown);

        m_pLocalPlayer.reset();
        m_pBuildingManager.reset();
        m_pInventoryManager.reset();
        m_pMissionManager.reset();

        m_World = UObjectWrapper();
        m_GameState = UObjectWrapper();
        m_GameMode = UObjectWrapper();

        m_bWorldReady = false;
        m_bPlayersLoaded = false;

        m_State = ESTWGameState::None;

        USS_LOG("STW GameMode shutdown complete");
    }

    void FSTWGameMode::Update()
    {
        // Called each tick - update managers
        if (m_pMissionManager)
            m_pMissionManager->Update();

        if (m_pBuildingManager)
            m_pBuildingManager->Update();
    }

    void FSTWGameMode::SetState(ESTWGameState NewState)
    {
        if (m_State == NewState)
            return;

        ESTWGameState OldState = m_State;
        m_State = NewState;

        USS_LOG("GameMode state: %s -> %s",
            GetGameStateName(OldState),
            GetGameStateName(NewState));

        // Notify callbacks
        for (const auto& Callback : m_StateChangeCallbacks)
        {
            Callback(OldState, NewState);
        }
    }

    void FSTWGameMode::SetLocalPlayer(std::unique_ptr<FSTWPlayerController> Player)
    {
        m_pLocalPlayer = std::move(Player);
    }

    void FSTWGameMode::RegisterStateChangeCallback(StateChangeCallback Callback)
    {
        m_StateChangeCallbacks.push_back(std::move(Callback));
    }

	// @timmie: replace with normal VFT / hooking system when available
    void FSTWGameMode::OnProcessEvent(void* Object, void* Function, void* Params)
    {
        if (!Function)
            return;

        // Get function name
        UFunctionWrapper FuncWrapper(Function);
        std::string FuncName = FuncWrapper.GetName();

        // Handle specific events based on current state
        if (FuncName.find("ReadyToStartMatch") != std::string::npos)
        {
            OnReadyToStartMatch();
        }
        else if (FuncName.find("ServerHandleMissionEvent_ToggledEditMode") != std::string::npos)
        {
            OnToggleEditMode(Params);
        }
        else if (FuncName.find("ServerHandleMissionEvent_StartLeavingZone") != std::string::npos)
        {
            OnStartLeavingZone(Params);
        }
        else if (FuncName.find("ServerCraftSchematic") != std::string::npos)
        {
            OnCraftSchematic(Params);
        }
        else if (FuncName.find("Tick") != std::string::npos)
        {
            Update();
        }

        // Forward to sub-managers
        if (m_pMissionManager)
            m_pMissionManager->OnProcessEvent(Object, Function, Params);

        if (m_pBuildingManager && m_pLocalPlayer)
            m_pBuildingManager->OnProcessEvent(Object, Function, Params);
    }

    void FSTWGameMode::OnReadyToStartMatch()
    {
        if (m_State != ESTWGameState::WaitingForWorld)
            return;

        USS_LOG("ReadyToStartMatch received");

        InitializeWorld();
        OnWorldReady();

        // Transition to waiting for players
        SetState(ESTWGameState::WaitingForPlayers);

        // For single-player, immediately proceed
        OnAllPlayersLoaded();
    }

    void FSTWGameMode::OnWorldReady()
    {
        USS_LOG("World is ready");
        m_bWorldReady = true;

        // Cache world references
        m_World = GetEngineCore().FindObjectByName("PersistentLevel");

        // Load husk assets into memory
        LoadHuskAssets();
    }

    void FSTWGameMode::OnAllPlayersLoaded()
    {
        if (m_bPlayersLoaded)
            return;

        USS_LOG("All players loaded");
        m_bPlayersLoaded = true;

        SetState(ESTWGameState::LoadingMission);

        // Spawn local player
        SpawnLocalPlayer();

        // Setup inventory
        SetupInventory();

        // Initialize mission
        InitializeMission();

        SetState(ESTWGameState::MissionActive);
    }

    void FSTWGameMode::OnMissionEvent(const char* EventName, void* Params)
    {
        USS_LOG("Mission event: %s", EventName);

        if (m_pMissionManager)
        {
            m_pMissionManager->OnMissionEvent(EventName, Params);
        }
    }

    void FSTWGameMode::OnToggleEditMode(void* Params)
    {
        USS_LOG("Edit mode toggled");

        // Toggle between build mode and normal mode
        if (m_pBuildingManager)
        {
            if (m_pBuildingManager->IsInBuildMode())
            {
                m_pBuildingManager->ExitBuildMode();
            }
            else
            {
                m_pBuildingManager->EnterBuildMode(EBuildingType::Wall);
            }
        }
    }

    void FSTWGameMode::OnStartLeavingZone(void* Params)
    {
        USS_LOG("Starting to leave zone");

        SetState(ESTWGameState::LeavingZone);

        // TODO: Handle zone exit
    }

    void FSTWGameMode::OnCraftSchematic(void* Params)
    {
        USS_LOG("Craft schematic requested");

        // TODO: Parse schematic ID from Params and call CraftItem
        // if (m_pInventoryManager)
        // {
        //     m_pInventoryManager->CraftItem(schematicId, 1);
        // }
    }

    void FSTWGameMode::InitializeWorld()
    {
        USS_LOG("Initializing world...");

        // TODO: Additional world initialization
        // - Patch gameplay abilities
        // - Setup replication
    }

    void FSTWGameMode::InitializeMission()
    {
        USS_LOG("Initializing mission...");

        if (!m_pMissionManager)
            return;

        // Create mission based on config
        FMissionConfig MissionConfig;
        MissionConfig.Type = m_Config.MissionType;
        MissionConfig.DifficultyLevel = m_Config.DifficultyLevel;
        MissionConfig.BlueprintPath = m_Config.MissionBlueprint;

        m_pMissionManager->StartMission(MissionConfig);
    }

    void FSTWGameMode::SpawnLocalPlayer()
    {
        USS_LOG("Spawning local player...");

        // Find local player controller from engine
        void* LocalController = GetEngineCore().FindLocalPlayerController();

        if (!LocalController)
        {
            USS_WARN("Local player controller not found yet");
            return;
        }

        // Create player controller wrapper
        m_pLocalPlayer = std::make_unique<FSTWPlayerController>(LocalController);

        if (!m_pLocalPlayer->IsValid())
        {
            USS_ERROR("Failed to wrap local player controller");
            m_pLocalPlayer.reset();
            return;
        }

        USS_LOG("Local player spawned successfully");
    }

    void FSTWGameMode::SetupInventory()
    {
        USS_LOG("Setting up inventory...");

        if (!m_pInventoryManager || !m_pLocalPlayer)
            return;

        // Re-initialize inventory with the player controller
        m_pInventoryManager->Initialize(m_pLocalPlayer->GetNative());
    }

    void FSTWGameMode::LoadHuskAssets()
    {
        USS_LOG("Loading husk assets into memory...");

        // Based on athena_plate.cpp husk loading
        const char* HuskAssets[] = {
            "/Game/Characters/Enemies/Husk/Blueprints/HuskPawn.HuskPawn_C",
            "/Game/Characters/Enemies/Husk/Blueprints/HuskPawn_Fire.HuskPawn_Fire_C",
            "/Game/Characters/Enemies/Husk/Blueprints/HuskPawn_Ice.HuskPawn_Ice_C",
            "/Game/Characters/Enemies/Husk/Blueprints/HuskPawn_Lightning.HuskPawn_Lightning_C",
            "/Game/Characters/Enemies/Husk/Blueprints/HuskPawn_Beehive.HuskPawn_Beehive_C",
            "/Game/Characters/Enemies/Husk/Blueprints/HuskPawn_Bombshell.HuskPawn_Bombshell_C",
            "/Game/Characters/Enemies/Husk/Blueprints/HuskPawn_Bombshell_Poison.HuskPawn_Bombshell_Poison_C",
            "/Game/Characters/Enemies/Husk/Blueprints/HuskPawn_Dwarf.HuskPawn_Dwarf_C",
            "/Game/Characters/Enemies/Husk/Blueprints/HuskPawn_Dwarf_Fire.HuskPawn_Dwarf_Fire_C",
            "/Game/Characters/Enemies/Husk/Blueprints/HuskPawn_Dwarf_Ice.HuskPawn_Dwarf_Ice_C",
            "/Game/Characters/Enemies/Husk/Blueprints/HuskPawn_Dwarf_Lightning.HuskPawn_Dwarf_Lightning_C",
        };

        // TODO: Implement FindOrLoadObject via engine core
        // For each asset, call engine's StaticLoadObject

        USS_LOG("Loaded %zu husk asset types", sizeof(HuskAssets) / sizeof(HuskAssets[0]));
    }

}