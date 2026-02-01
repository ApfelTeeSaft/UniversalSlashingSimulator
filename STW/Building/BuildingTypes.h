/**
 * UniversalSlashingSimulator - Building System Types
 *
 * Defines STW building structures, traps, and construction types.
 */

#pragma once

#include "../../Core/Common.h"
#include <string>
#include <vector>

namespace USS
{
    /**
     * Building piece types
     */
    enum class EBuildingType : uint8
    {
        None = 0,
        Wall,
        Floor,
        Ramp,
        Roof,
        Stair,

        // STW-specific
        Trap,
        StructuralBuild,    // Mission objectives like radar towers
        DefenseBuild,       // Atlas, amplifiers, etc.
    };

    /**
     * Building material tiers
     */
    enum class EBuildingMaterial : uint8
    {
        Wood = 0,
        Stone,
        Metal,

        // Special materials (STW)
        Tier2,      // Upgraded wood
        Tier3,      // Upgraded stone/brick
        Tier4,      // Upgraded metal
    };

    /**
     * Building upgrade level (STW progression)
     */
    enum class EBuildingTier : uint8
    {
        Tier1 = 1,
        Tier2 = 2,
        Tier3 = 3,
    };

    /**
     * Trap types in STW
     */
    enum class ETrapType : uint8
    {
        None = 0,

        // Floor traps
        FloorSpikes,
        FloorFreeze,
        FloorLauncher,
        FloorElectric,
        FloorRetractSpikes,
        FloorBroadside,

        // Wall traps
        WallDarts,
        WallDynamo,
        WallLights,
        WallLauncher,
        WallSpikes,

        // Ceiling traps
        CeilingZapper,
        CeilingGas,
        CeilingDropTrap,
        CeilingElectric,
        CeilingTire,

        // Defender posts
        DefenderPost,
    };

    /**
     * Trap targeting behavior
     */
    enum class ETrapTargeting : uint8
    {
        None = 0,
        Proximity,      // Triggers when enemy is near
        Path,           // Targets pathing enemies
        Random,         // Random targeting
        Strongest,      // Targets highest HP
        Closest,        // Targets closest enemy
    };

    /**
     * Building state
     */
    enum class EBuildingState : uint8
    {
        None = 0,
        Previewing,     // Ghost preview before placement
        Placing,        // In placement mode
        Building,       // Construction in progress
        Built,          // Fully constructed
        Damaged,        // Has taken damage
        Upgrading,      // Being upgraded
        Repairing,      // Being repaired
        Destroying,     // Being demolished
    };

    /**
     * Resource cost for building
     */
    struct FBuildingCost
    {
        int32 WoodCost = 0;
        int32 StoneCost = 0;
        int32 MetalCost = 0;

        // STW ingredients
        int32 NutsAndBolts = 0;
        int32 PlanksCount = 0;
        int32 RoughOre = 0;

        bool CanAfford(int32 Wood, int32 Stone, int32 Metal) const
        {
            return Wood >= WoodCost && Stone >= StoneCost && Metal >= MetalCost;
        }
    };

    /**
     * Building piece stats
     */
    struct FBuildingStats
    {
        float MaxHealth = 100.0f;
        float CurrentHealth = 100.0f;
        float BuildTime = 3.0f;         // Seconds to build
        float RepairRate = 10.0f;       // Health per second when repairing
        float DamageResistance = 0.0f;  // Percentage damage reduction

        // Material-specific modifiers
        float FireResistance = 0.0f;
        float PhysicalResistance = 0.0f;
        float EnergyResistance = 0.0f;
    };

    /**
     * Trap stats
     */
    struct FTrapStats
    {
        float Damage = 0.0f;
        float ReloadTime = 0.0f;        // Seconds between activations
        float Range = 0.0f;             // Trigger range
        float KnockbackForce = 0.0f;
        float SlowPercent = 0.0f;       // Movement slow on hit
        float StunDuration = 0.0f;

        int32 MaxDurability = 0;
        int32 CurrentDurability = 0;
        int32 UsesPerActivation = 1;    // How many uses per trigger

        ETrapTargeting Targeting = ETrapTargeting::Proximity;

        // Status effects
        bool bAppliesAffliction = false;
        bool bAppliesSnare = false;
        bool bAppliesStun = false;
        bool bAppliesFreeze = false;
    };

    /**
     * Building piece data
     */
    struct FBuildingPiece
    {
        std::string BuildingId;
        EBuildingType Type = EBuildingType::None;
        EBuildingMaterial Material = EBuildingMaterial::Wood;
        EBuildingTier Tier = EBuildingTier::Tier1;
        EBuildingState State = EBuildingState::None;

        FBuildingStats Stats;
        FBuildingCost Cost;

        // Grid position
        int32 GridX = 0;
        int32 GridY = 0;
        int32 GridZ = 0;
        float Rotation = 0.0f;

        // Owner info
        std::string OwnerId;
        bool bIsPlayerBuilt = false;

        // Native actor reference
        void* BuildingActor = nullptr;

        float GetHealthPercent() const
        {
            return (Stats.MaxHealth > 0.0f) ? (Stats.CurrentHealth / Stats.MaxHealth) : 0.0f;
        }

        bool IsFullHealth() const
        {
            return Stats.CurrentHealth >= Stats.MaxHealth;
        }

        bool IsDamaged() const
        {
            return Stats.CurrentHealth < Stats.MaxHealth;
        }
    };

    /**
     * Trap instance data
     */
    struct FTrapInstance
    {
        std::string TrapId;
        ETrapType Type = ETrapType::None;
        EBuildingState State = EBuildingState::None;

        FTrapStats Stats;
        FBuildingCost Cost;

        // Attached building
        std::string AttachedBuildingId;

        // Placement
        int32 GridX = 0;
        int32 GridY = 0;
        int32 GridZ = 0;
        float Rotation = 0.0f;

        // State
        float CooldownRemaining = 0.0f;
        int32 TotalKills = 0;
        bool bIsArmed = true;
        bool bIsTriggered = false;

        // Owner
        std::string OwnerId;

        // Native reference
        void* TrapActor = nullptr;

        bool IsReady() const
        {
            return bIsArmed && CooldownRemaining <= 0.0f && Stats.CurrentDurability > 0;
        }

        float GetDurabilityPercent() const
        {
            return (Stats.MaxDurability > 0) ?
                   (static_cast<float>(Stats.CurrentDurability) / Stats.MaxDurability) : 0.0f;
        }
    };

    /**
     * Build preview ghost data
     */
    struct FBuildPreview
    {
        EBuildingType Type = EBuildingType::None;
        EBuildingMaterial Material = EBuildingMaterial::Wood;

        // Preview position
        float LocationX = 0.0f;
        float LocationY = 0.0f;
        float LocationZ = 0.0f;
        float Rotation = 0.0f;

        // Validity
        bool bIsValidPlacement = false;
        bool bCanAfford = false;
        bool bIsOverlapping = false;
        bool bIsFloating = false;

        FBuildingCost Cost;
    };

    /**
     * Building change event data
     */
    struct FBuildingChangeEvent
    {
        enum class EChangeType
        {
            Built,
            Destroyed,
            Damaged,
            Repaired,
            Upgraded,
            TrapPlaced,
            TrapTriggered,
            TrapDestroyed,
        };

        EChangeType Type;
        std::string BuildingId;
        std::string PlayerId;

        float OldHealth = 0.0f;
        float NewHealth = 0.0f;
        float Damage = 0.0f;

        void* DamageCauser = nullptr;
    };

    /**
     * Default building costs by material
     */
    inline FBuildingCost GetDefaultBuildCost(EBuildingMaterial Material)
    {
        FBuildingCost Cost;

        switch (Material)
        {
        case EBuildingMaterial::Wood:
            Cost.WoodCost = 10;
            break;
        case EBuildingMaterial::Stone:
            Cost.StoneCost = 10;
            break;
        case EBuildingMaterial::Metal:
            Cost.MetalCost = 10;
            break;
        default:
            Cost.WoodCost = 10;
            break;
        }

        return Cost;
    }

    /**
     * Default building stats by material
     */
    inline FBuildingStats GetDefaultBuildStats(EBuildingMaterial Material)
    {
        FBuildingStats Stats;

        switch (Material)
        {
        case EBuildingMaterial::Wood:
            Stats.MaxHealth = 150.0f;
            Stats.BuildTime = 3.0f;
            Stats.FireResistance = -0.25f;  // Weak to fire
            break;
        case EBuildingMaterial::Stone:
            Stats.MaxHealth = 300.0f;
            Stats.BuildTime = 4.0f;
            Stats.PhysicalResistance = 0.1f;
            break;
        case EBuildingMaterial::Metal:
            Stats.MaxHealth = 500.0f;
            Stats.BuildTime = 5.0f;
            Stats.PhysicalResistance = 0.15f;
            Stats.EnergyResistance = -0.25f;  // Weak to energy
            break;
        default:
            Stats.MaxHealth = 150.0f;
            Stats.BuildTime = 3.0f;
            break;
        }

        Stats.CurrentHealth = Stats.MaxHealth;
        return Stats;
    }

}