/**
 * UniversalSlashingSimulator - Mission Types
 *
 * Types and enums for the STW mission system.
 */

#pragma once

#include "../../Core/Common.h"
#include <string>
#include <vector>

namespace USS
{
    // Mission types from STW
    enum class EMissionType : uint8
    {
        Unknown = 0,

        // Core mission types
        FarmsteadDefense,           // Defend the base (Homebase)
        SurvivorsRescue,            // Rescue survivors
        EncampmentDestroy,          // Destroy encampments
        EncampmentDestruction,      // Alias for EncampmentDestroy
        RadarBuild,                 // Build radar towers
        RadarGridConstruction,      // Alias for RadarBuild
        DataRetrieval,              // Retrieve data

        // Storm Shield
        StormShieldDefense,         // SSD missions

        // Special types
        EliminateAndCollect,        // Kill and collect
        DeliverTheBomb,             // DTB mission
        RepairTheShelter,           // RTS mission
        EvacuateTheShelter,         // Evac mission
        RideTheLightning,           // RTL mission
        CategoryStorm,              // 4-atlas
        RetrieveTheData,            // RTD mission
        LaunchTheBalloon,           // LTB mission

        // Horde modes
        ChallengeTheHorde,
        FrostniteEndurance,

        // Wargames
        Wargames,

        Max
    };

    // Mission state
    enum class EMissionState : uint8
    {
        None,
        Loading,
        Setup,              // Players setting up defenses
        Active,             // Mission in progress
        DefensePhase,       // Defending objective
        Intermission,       // Between waves
        Completed,
        Complete = Completed, // Alias
        Failed,
        Abandoned
    };

    // Objective state
    enum class EObjectiveState : uint8
    {
        Inactive,
        Active,
        Completed,
        Failed,
        Optional
    };

    // Objective type
    enum class EObjectiveType : uint8
    {
        Unknown = 0,
        DefendLocation,     // Defend a specific point
        Defend = DefendLocation, // Alias
        KillEnemies,        // Kill X enemies
        Kill = KillEnemies, // Alias
        CollectItems,       // Collect X items
        Collect = CollectItems, // Alias
        BuildStructures,    // Build X structures
        Build = BuildStructures, // Alias
        RescueSurvivors,    // Rescue X survivors
        Rescue = RescueSurvivors, // Alias
        DestroyObjects,     // Destroy X objects
        EscortPayload,      // Escort/deliver payload
        ActivateDevice,     // Activate a device
        SurviveWaves,       // Survive X waves
        Timer,              // Time-based objective
        Explore,            // Exploration objective
    };

    // Difficulty tier
    enum class EDifficultyTier : uint8
    {
        VeryLow = 0,
        Low,
        Medium,
        High,
        VeryHigh,
        Extreme,
        Max
    };

    // Mission alert type
    enum class EMissionAlert : uint8
    {
        None = 0,
        Storm,
        MiniBeoss,
        ElementalModifier,
        MutantStorm,
        GroupMission,
    };

    // Mission configuration
    struct FMissionConfig
    {
        EMissionType Type;
        std::string BlueprintPath;      // e.g., "Mission_FarmsteadFort_C"
        std::string ZoneName;
        int32 DifficultyLevel;          // 1-140+
        EDifficultyTier DifficultyTier;
        EMissionAlert AlertType;

        // Time limits (seconds, 0 = no limit)
        int32 SetupTimeLimit;
        int32 MissionTimeLimit;

        // Wave settings
        int32 WaveCount;
        int32 MaxWaves;                 // Alias for WaveCount
        int32 EnemiesPerWave;

        FMissionConfig()
            : Type(EMissionType::Unknown)
            , DifficultyLevel(1)
            , DifficultyTier(EDifficultyTier::Low)
            , AlertType(EMissionAlert::None)
            , SetupTimeLimit(0)
            , MissionTimeLimit(0)
            , WaveCount(1)
            , MaxWaves(1)
            , EnemiesPerWave(20)
        {}
    };

    // Objective definition
    struct FObjectiveDefinition
    {
        EObjectiveType Type;
        std::string Name;
        std::string DisplayText;        // Display name for UI
        std::string Description;

        int32 TargetCount;          // How many to complete
        int32 CurrentCount;         // Current progress
        float TimeLimit;            // Time limit in seconds (0 = no limit)

        bool bIsRequired;           // Required for mission success
        bool bIsPrimary;            // Primary objective
        bool bIsBonus;              // Bonus objective

        EObjectiveState State;

        FObjectiveDefinition()
            : Type(EObjectiveType::Unknown)
            , TargetCount(1)
            , CurrentCount(0)
            , TimeLimit(0.0f)
            , bIsRequired(true)
            , bIsPrimary(true)
            , bIsBonus(false)
            , State(EObjectiveState::Inactive)
        {}

        float GetProgress() const
        {
            if (TargetCount <= 0) return 0.0f;
            return static_cast<float>(CurrentCount) / static_cast<float>(TargetCount);
        }

        bool IsComplete() const
        {
            return CurrentCount >= TargetCount;
        }
    };

    // Wave info
    struct FWaveInfo
    {
        int32 WaveNumber;
        int32 CurrentWave;          // Alias for WaveNumber
        int32 TotalWaves;
        int32 MaxWaves;             // Alias for TotalWaves
        int32 EnemiesRemaining;
        int32 EnemiesTotal;
        int32 EnemiesSpawned;       // Total enemies spawned so far
        float TimeRemaining;        // Seconds
        double WaveStartTime;       // When this wave started
        bool bIsDefenseWave;
        bool bIsActive;             // Is wave currently active

        FWaveInfo()
            : WaveNumber(0)
            , CurrentWave(0)
            , TotalWaves(1)
            , MaxWaves(1)
            , EnemiesRemaining(0)
            , EnemiesTotal(0)
            , EnemiesSpawned(0)
            , TimeRemaining(0.0f)
            , WaveStartTime(0.0)
            , bIsDefenseWave(false)
            , bIsActive(false)
        {}
    };

    // Mission rewards
    struct FMissionReward
    {
        std::string ItemId;
        int32 Quantity;
        int32 Rarity;               // 0=common, 1=uncommon, etc.
        bool bIsBonusReward;

        FMissionReward()
            : Quantity(1)
            , Rarity(0)
            , bIsBonusReward(false)
        {}
    };

    // Mission result
    struct FMissionResult
    {
        EMissionState FinalState;
        bool bSuccess;              // Did mission succeed
        float CompletionPercentage;
        int32 ScoreEarned;
        int32 FinalScore;           // Alias for ScoreEarned
        int32 XPEarned;

        // Wave completion stats
        int32 WavesCompleted;
        int32 TotalWaves;

        // Objective stats
        int32 ObjectivesCompleted;
        int32 TotalObjectives;

        std::vector<FMissionReward> Rewards;

        FMissionResult()
            : FinalState(EMissionState::None)
            , bSuccess(false)
            , CompletionPercentage(0.0f)
            , ScoreEarned(0)
            , FinalScore(0)
            , XPEarned(0)
            , WavesCompleted(0)
            , TotalWaves(0)
            , ObjectivesCompleted(0)
            , TotalObjectives(0)
        {}
    };

    // Helper functions
    inline const char* GetMissionTypeName(EMissionType Type)
    {
        switch (Type)
        {
        case EMissionType::FarmsteadDefense:    return "Defend the Base";
        case EMissionType::SurvivorsRescue:     return "Rescue the Survivors";
        case EMissionType::EncampmentDestroy:   return "Destroy the Encampments";
        case EMissionType::RadarBuild:          return "Build the Radar";
        case EMissionType::DataRetrieval:       return "Retrieve the Data";
        case EMissionType::StormShieldDefense:  return "Storm Shield Defense";
        case EMissionType::DeliverTheBomb:      return "Deliver the Bomb";
        case EMissionType::RepairTheShelter:    return "Repair the Shelter";
        case EMissionType::EvacuateTheShelter:  return "Evacuate the Shelter";
        case EMissionType::RideTheLightning:    return "Ride the Lightning";
        case EMissionType::CategoryStorm:       return "Category Storm";
        case EMissionType::RetrieveTheData:     return "Retrieve the Data";
        case EMissionType::LaunchTheBalloon:    return "Launch the Balloon";
        case EMissionType::ChallengeTheHorde:   return "Challenge the Horde";
        case EMissionType::FrostniteEndurance:  return "Frostnite";
        case EMissionType::Wargames:            return "Wargames";
        default:                                return "Unknown";
        }
    }

    inline const char* GetMissionStateName(EMissionState State)
    {
        switch (State)
        {
        case EMissionState::None:           return "None";
        case EMissionState::Loading:        return "Loading";
        case EMissionState::Setup:          return "Setup";
        case EMissionState::Active:         return "Active";
        case EMissionState::DefensePhase:   return "Defense";
        case EMissionState::Intermission:   return "Intermission";
        case EMissionState::Completed:      return "Completed";
        case EMissionState::Failed:         return "Failed";
        case EMissionState::Abandoned:      return "Abandoned";
        default:                            return "Unknown";
        }
    }

}