/**
 * UniversalSlashingSimulator - STW Game Mode
 *
 * Core STW game mode controller. Manages the game lifecycle,
 * coordinates systems, and handles ProcessEvent routing.
 *
 * Based on FarmsteadPlate from PolarisV2-STW.
 *
 * Lifecycle:
 * 1. Initialize() - Called after engine core ready
 * 2. OnWorldReady() - Called when world is loaded
 * 3. OnPlayersLoaded() - Called when all players joined
 * 4. OnMissionStart() - Mission gameplay begins
 * 5. OnMissionEnd() - Mission complete/failed
 * 6. Shutdown() - Cleanup
 */

#pragma once

#include "../../Core/Common.h"
#include "../../Engine/UObject/UObjectWrapper.h"
#include "../Missions/MissionTypes.h"
#include <functional>

namespace USS
{
    // Forward declarations
    class FMissionManager;
    class FInventoryManager;
    class FBuildingManager;
    class FSTWPlayerController;

    // Game mode state
    enum class ESTWGameState : uint8
    {
        None,
        Initializing,
        WaitingForWorld,
        WaitingForPlayers,
        LoadingMission,
        MissionActive,
        MissionComplete,
        MissionFailed,
        LeavingZone,
        Shutdown
    };

    // Game mode configuration
    struct FSTWGameConfig
    {
        // Zone configuration
        std::string ZoneName;           // e.g., "Zone_Onboarding_FarmsteadFort"
        std::string MissionBlueprint;   // e.g., "Mission_FarmsteadFort_C"
        std::string MapName;            // Map to load

        // Player settings
        int32 MaxPlayers;
        bool bAllowBots;

        // Mission settings
        EMissionType MissionType;
        int32 DifficultyLevel;
        int32 DefaultDifficulty;        // Default difficulty for the zone

        // Feature toggles
        bool bEnableMissions;
        bool bEnableInventory;
        bool bEnableBuilding;

        FSTWGameConfig()
            : MaxPlayers(4)
            , bAllowBots(true)
            , MissionType(EMissionType::FarmsteadDefense)
            , DifficultyLevel(1)
            , DefaultDifficulty(1)
            , bEnableMissions(true)
            , bEnableInventory(true)
            , bEnableBuilding(true)
        {
        }
    };

    // STW Game Mode
    class FSTWGameMode
    {
    public:
        USS_NON_COPYABLE(FSTWGameMode)
            USS_NON_MOVABLE(FSTWGameMode)

            static FSTWGameMode& Get();

        // Lifecycle
        EResult Initialize(const FSTWGameConfig& Config);
        void Shutdown();
        void Update();  // Called each tick

        // State
        ESTWGameState GetState() const { return m_State; }
        bool IsInitialized() const { return m_State != ESTWGameState::None; }
        bool IsMissionActive() const { return m_State == ESTWGameState::MissionActive; }

        // Configuration
        const FSTWGameConfig& GetConfig() const { return m_Config; }

        // Manager access
        FMissionManager* GetMissionManager() const { return m_pMissionManager.get(); }
        FInventoryManager* GetInventoryManager() const { return m_pInventoryManager.get(); }
        FBuildingManager* GetBuildingManager() const { return m_pBuildingManager.get(); }

        // Player management
        FSTWPlayerController* GetLocalPlayer() const { return m_pLocalPlayer.get(); }
        void SetLocalPlayer(std::unique_ptr<FSTWPlayerController> Player);

        // Event handlers (called from ProcessEvent hook)
        void OnProcessEvent(void* Object, void* Function, void* Params);

        // Callbacks for external systems
        using StateChangeCallback = std::function<void(ESTWGameState OldState, ESTWGameState NewState)>;
        void RegisterStateChangeCallback(StateChangeCallback Callback);

    private:
        FSTWGameMode();
        ~FSTWGameMode();

        // State transitions
        void SetState(ESTWGameState NewState);

        // Event handlers
        void OnReadyToStartMatch();
        void OnWorldReady();
        void OnAllPlayersLoaded();
        void OnMissionEvent(const char* EventName, void* Params);
        void OnToggleEditMode(void* Params);
        void OnStartLeavingZone(void* Params);
        void OnCraftSchematic(void* Params);

        // Initialization helpers
        void InitializeWorld();
        void InitializeMission();
        void SpawnLocalPlayer();
        void SetupInventory();
        void LoadHuskAssets();

        // State
        ESTWGameState m_State;
        FSTWGameConfig m_Config;
        bool m_bWorldReady;
        bool m_bPlayersLoaded;

        // Managers
        std::unique_ptr<FMissionManager> m_pMissionManager;
        std::unique_ptr<FInventoryManager> m_pInventoryManager;
        std::unique_ptr<FBuildingManager> m_pBuildingManager;

        // Player
        std::unique_ptr<FSTWPlayerController> m_pLocalPlayer;

        // Callbacks
        std::vector<StateChangeCallback> m_StateChangeCallbacks;

        // Engine references (cached)
        UObjectWrapper m_World;
        UObjectWrapper m_GameState;
        UObjectWrapper m_GameMode;
    };

    // Convenience function
    inline FSTWGameMode& GetSTWGameMode()
    {
        return FSTWGameMode::Get();
    }

    // State name helper
    inline const char* GetGameStateName(ESTWGameState State)
    {
        switch (State)
        {
        case ESTWGameState::None:              return "None";
        case ESTWGameState::Initializing:      return "Initializing";
        case ESTWGameState::WaitingForWorld:   return "WaitingForWorld";
        case ESTWGameState::WaitingForPlayers: return "WaitingForPlayers";
        case ESTWGameState::LoadingMission:    return "LoadingMission";
        case ESTWGameState::MissionActive:     return "MissionActive";
        case ESTWGameState::MissionComplete:   return "MissionComplete";
        case ESTWGameState::MissionFailed:     return "MissionFailed";
        case ESTWGameState::LeavingZone:       return "LeavingZone";
        case ESTWGameState::Shutdown:          return "Shutdown";
        default:                               return "Unknown";
        }
    }

}