/**
 * UniversalSlashingSimulator - Mission Manager
 *
 * Manages mission lifecycle, objectives, waves, and scoring.
 * Based on AFortMission and AFortMissionManager from STW.
 */

#pragma once

#include "../../Core/Common.h"
#include "../../Engine/UObject/UObjectWrapper.h"
#include "MissionTypes.h"
#include "MissionObjective.h"
#include <vector>
#include <memory>
#include <functional>

namespace USS
{
    // Mission event callback
    using FMissionEventCallback = std::function<void(const char* EventName, void* Params)>;

    class FMissionManager
    {
    public:
        FMissionManager();
        ~FMissionManager();

        USS_NON_COPYABLE(FMissionManager)
        USS_NON_MOVABLE(FMissionManager)

        // Lifecycle
        EResult Initialize();
        void Shutdown();
        void Update();

        // Mission control
        EResult StartMission(const FMissionConfig& Config);
        void EndMission(bool bSuccess);
        void AbortMission();

        // State
        EMissionState GetState() const { return m_State; }
        const FMissionConfig& GetConfig() const { return m_Config; }
        bool IsActive() const { return m_State == EMissionState::Active || m_State == EMissionState::DefensePhase; }

        // Objectives
        void AddObjective(std::unique_ptr<FMissionObjective> Objective);
        FMissionObjective* GetObjective(int32 Index) const;
        int32 GetObjectiveCount() const { return static_cast<int32>(m_Objectives.size()); }
        void UpdateObjectiveProgress(int32 Index, int32 Progress);

        // Waves
        void StartWave(int32 WaveNumber);
        void EndWave();
        const FWaveInfo& GetWaveInfo() const { return m_WaveInfo; }

        // Scoring
        int32 GetScore() const { return m_Score; }
        void AddScore(int32 Points);

        // Events
        void OnProcessEvent(void* Object, void* Function, void* Params);
        void OnMissionEvent(const char* EventName, void* Params);

        // Callbacks
        void RegisterEventCallback(FMissionEventCallback Callback);

        // Result
        FMissionResult GetResult() const;

    private:
        void SetState(EMissionState NewState);
        void CreateDefaultObjectives();
        void CheckObjectivesComplete();
        void CalculateResult();

        // State
        EMissionState m_State;
        FMissionConfig m_Config;
        FWaveInfo m_WaveInfo;
        int32 m_Score;

        // Objectives
        std::vector<std::unique_ptr<FMissionObjective>> m_Objectives;

        // Engine references (cached)
        UObjectWrapper m_MissionActor;       // AFortMission*
        UObjectWrapper m_MissionManagerActor; // AFortMissionManager*
        UObjectWrapper m_AIDirector;         // AFortAIDirector*

        // Callbacks
        std::vector<FMissionEventCallback> m_EventCallbacks;

        // Result
        FMissionResult m_Result;
    };

}