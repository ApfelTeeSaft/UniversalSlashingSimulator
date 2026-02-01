/**
 * UniversalSlashingSimulator - Building Manager
 *
 * Manages STW building system: construction, traps, upgrades, and repairs.
 */

#pragma once

#include "BuildingTypes.h"
#include "../../Engine/UObject/UObjectWrapper.h"
#include <functional>
#include <unordered_map>
#include <memory>

namespace USS
{
    // Forward declarations
    class FInventoryManager;

    /**
     * Callback for building events
     */
    using FBuildingEventCallback = std::function<void(const FBuildingChangeEvent&)>;

    /**
     * Building Manager
     *
     * Handles all building operations including:
     * - Building placement and preview
     * - Material selection and switching
     * - Trap placement and management
     * - Building upgrades (STW tiers)
     * - Repair and demolition
     * - Build limit tracking
     */
    class FBuildingManager
    {
    public:
        FBuildingManager();
        ~FBuildingManager();

        // Lifecycle
        EResult Initialize(FInventoryManager* InventoryManager);
        void Shutdown();
        void Update();

        // ===========================================
        // Building Operations
        // ===========================================

        /**
         * Enter build mode for a specific type
         */
        EResult EnterBuildMode(EBuildingType Type);

        /**
         * Exit build mode
         */
        void ExitBuildMode();

        /**
         * Check if currently in build mode
         */
        bool IsInBuildMode() const { return m_bIsInBuildMode; }

        /**
         * Get current build type being previewed
         */
        EBuildingType GetCurrentBuildType() const { return m_CurrentBuildType; }

        /**
         * Set building material
         */
        void SetBuildMaterial(EBuildingMaterial Material);

        /**
         * Get current building material
         */
        EBuildingMaterial GetBuildMaterial() const { return m_CurrentMaterial; }

        /**
         * Cycle to next material
         */
        void CycleMaterial();

        /**
         * Update build preview position
         */
        void UpdateBuildPreview(float LocationX, float LocationY, float LocationZ, float Rotation);

        /**
         * Get current build preview
         */
        const FBuildPreview& GetBuildPreview() const { return m_BuildPreview; }

        /**
         * Confirm placement at current preview location
         */
        EResult ConfirmBuild();

        /**
         * Cancel current build operation
         */
        void CancelBuild();

        // ===========================================
        // Building Access
        // ===========================================

        /**
         * Get building by ID
         */
        const FBuildingPiece* GetBuilding(const std::string& BuildingId) const;

        /**
         * Get building at grid position
         */
        const FBuildingPiece* GetBuildingAtGrid(int32 X, int32 Y, int32 Z) const;

        /**
         * Get all buildings owned by player
         */
        std::vector<const FBuildingPiece*> GetPlayerBuildings(const std::string& PlayerId) const;

        /**
         * Get total building count
         */
        int32 GetBuildingCount() const { return static_cast<int32>(m_Buildings.size()); }

        /**
         * Get player's building count
         */
        int32 GetPlayerBuildingCount(const std::string& PlayerId) const;

        // ===========================================
        // Building Modifications
        // ===========================================

        /**
         * Damage a building
         */
        EResult DamageBuilding(const std::string& BuildingId, float Damage, void* DamageCauser);

        /**
         * Repair a building
         */
        EResult RepairBuilding(const std::string& BuildingId, float Amount);

        /**
         * Upgrade building to next tier (STW)
         */
        EResult UpgradeBuilding(const std::string& BuildingId);

        /**
         * Demolish a building (refund partial materials)
         */
        EResult DemolishBuilding(const std::string& BuildingId);

        /**
         * Edit building piece (change type/shape)
         */
        EResult EditBuilding(const std::string& BuildingId, EBuildingType NewType);

        // ===========================================
        // Trap Operations
        // ===========================================

        /**
         * Enter trap placement mode
         */
        EResult EnterTrapPlacementMode(ETrapType TrapType, const std::string& TrapItemId);

        /**
         * Exit trap placement mode
         */
        void ExitTrapPlacementMode();

        /**
         * Check if in trap placement mode
         */
        bool IsInTrapPlacementMode() const { return m_bIsPlacingTrap; }

        /**
         * Confirm trap placement
         */
        EResult ConfirmTrapPlacement(const std::string& AttachedBuildingId);

        /**
         * Get trap by ID
         */
        const FTrapInstance* GetTrap(const std::string& TrapId) const;

        /**
         * Get traps attached to building
         */
        std::vector<const FTrapInstance*> GetTrapsOnBuilding(const std::string& BuildingId) const;

        /**
         * Trigger trap manually (for testing)
         */
        EResult TriggerTrap(const std::string& TrapId);

        /**
         * Reload/reset trap
         */
        EResult ReloadTrap(const std::string& TrapId);

        // ===========================================
        // STW-Specific Features
        // ===========================================

        /**
         * Get current build limit (based on constructor bonus, etc.)
         */
        int32 GetBuildLimit() const { return m_BuildLimit; }

        /**
         * Set build limit
         */
        void SetBuildLimit(int32 Limit) { m_BuildLimit = Limit; }

        /**
         * Check if at build limit
         */
        bool IsAtBuildLimit() const;

        /**
         * Get building speed multiplier (constructor bonus)
         */
        float GetBuildSpeedMultiplier() const { return m_BuildSpeedMultiplier; }

        /**
         * Set building speed multiplier
         */
        void SetBuildSpeedMultiplier(float Multiplier) { m_BuildSpeedMultiplier = Multiplier; }

        /**
         * Get trap damage multiplier (constructor bonus)
         */
        float GetTrapDamageMultiplier() const { return m_TrapDamageMultiplier; }

        /**
         * Set trap damage multiplier
         */
        void SetTrapDamageMultiplier(float Multiplier) { m_TrapDamageMultiplier = Multiplier; }

        /**
         * Apply constructor perks
         */
        void ApplyConstructorPerks(float BuildSpeed, float TrapDamage, int32 ExtraBuildLimit);

        // ===========================================
        // Events
        // ===========================================

        /**
         * Register callback for building events
         */
        void RegisterEventCallback(FBuildingEventCallback Callback);

        /**
         * Handle ProcessEvent for building-related functions
         */
        void OnProcessEvent(void* Object, void* Function, void* Params);

    private:
        // Internal helpers
        std::string GenerateBuildingId() const;
        std::string GenerateTrapId() const;

        bool ValidatePlacement(const FBuildPreview& Preview) const;
        bool CheckOverlap(float X, float Y, float Z) const;
        bool CheckSupport(float X, float Y, float Z, EBuildingType Type) const;

        void NotifyChange(const FBuildingChangeEvent& Event);

        void UpdateTraps(float DeltaTime);
        void ProcessTrapTrigger(FTrapInstance& Trap);

        // State
        bool m_bIsInBuildMode = false;
        bool m_bIsPlacingTrap = false;

        EBuildingType m_CurrentBuildType = EBuildingType::None;
        EBuildingMaterial m_CurrentMaterial = EBuildingMaterial::Wood;
        ETrapType m_CurrentTrapType = ETrapType::None;
        std::string m_CurrentTrapItemId;

        FBuildPreview m_BuildPreview;

        // Building storage
        std::unordered_map<std::string, FBuildingPiece> m_Buildings;
        std::unordered_map<std::string, FTrapInstance> m_Traps;

        // Grid lookup (for fast position-based queries)
        std::unordered_map<uint64, std::string> m_GridToBuildingId;

        // ID counters
        mutable uint32 m_BuildingIdCounter = 0;
        mutable uint32 m_TrapIdCounter = 0;

        // STW bonuses
        int32 m_BuildLimit = 1000;
        float m_BuildSpeedMultiplier = 1.0f;
        float m_TrapDamageMultiplier = 1.0f;

        // References
        FInventoryManager* m_pInventoryManager = nullptr;

        UObjectWrapper m_BuildingManagerActor;
        UObjectWrapper m_TrapManagerActor;

        // Event callbacks
        std::vector<FBuildingEventCallback> m_EventCallbacks;
    };

    /**
     * Global building manager accessor
     */
    FBuildingManager& GetLocalBuildingManager();

}