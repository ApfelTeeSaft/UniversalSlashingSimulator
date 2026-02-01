/**
 * UniversalSlashingSimulator - Mission Manager Implementation
 */

#include "MissionManager.h"
#include "MissionObjective.h"
#include "../../Core/Logging/Log.h"
#include "../../Engine/EngineCore.h"

namespace USS
{
    FMissionManager::FMissionManager()
        : m_State(EMissionState::None)
        , m_Score(0)
    {
        m_WaveInfo = {};
    }

    FMissionManager::~FMissionManager()
    {
        Shutdown();
    }

    EResult FMissionManager::Initialize()
    {
        USS_LOG("Initializing Mission Manager...");

        m_State = EMissionState::None;
        m_Objectives.clear();
        m_EventCallbacks.clear();
        m_Score = 0;

        // Cache engine references
        // TODO: Find AFortMissionManager in world
        m_MissionManagerActor = GetEngineCore().FindObjectByName("FortMissionManager");

        USS_LOG("Mission Manager initialized");
        return EResult::Success;
    }

    void FMissionManager::Shutdown()
    {
        if (m_State != EMissionState::None)
        {
            AbortMission();
        }

        m_Objectives.clear();
        m_EventCallbacks.clear();
        m_MissionActor = UObjectWrapper();
        m_MissionManagerActor = UObjectWrapper();
        m_AIDirector = UObjectWrapper();
    }

    void FMissionManager::Update()
    {
        if (!IsActive())
            return;

        // Update all objectives
        for (auto& Objective : m_Objectives)
        {
            if (Objective)
            {
                Objective->Update();
            }
        }

        // Check for mission completion
        CheckObjectivesComplete();
    }

    EResult FMissionManager::StartMission(const FMissionConfig& Config)
    {
        if (m_State != EMissionState::None && m_State != EMissionState::Complete &&
            m_State != EMissionState::Failed)
        {
            USS_WARN("Cannot start mission - already active");
            return EResult::InvalidState;
        }

        USS_LOG("Starting mission: %s (Type: %d, Difficulty: %d)",
            Config.MissionName.c_str(),
            static_cast<int>(Config.Type),
            Config.Difficulty);

        m_Config = Config;
        m_Score = 0;
        m_Objectives.clear();

        // Initialize wave info
        m_WaveInfo.CurrentWave = 0;
        m_WaveInfo.MaxWaves = Config.MaxWaves > 0 ? Config.MaxWaves : 1;
        m_WaveInfo.EnemiesRemaining = 0;
        m_WaveInfo.EnemiesSpawned = 0;
        m_WaveInfo.bIsActive = false;

        // Create default objectives based on mission type
        CreateDefaultObjectives();

        SetState(EMissionState::Loading);

        // TODO: Would spawn mission actors, initialize AI director, etc.
        // For now, transition directly to active
        SetState(EMissionState::Active);

        USS_LOG("Mission started with %d objectives", static_cast<int>(m_Objectives.size()));

        return EResult::Success;
    }

    void FMissionManager::EndMission(bool bSuccess)
    {
        if (!IsActive() && m_State != EMissionState::Loading)
        {
            USS_WARN("Cannot end mission - not active");
            return;
        }

        USS_LOG("Ending mission: %s", bSuccess ? "SUCCESS" : "FAILURE");

        // End any active wave
        if (m_WaveInfo.bIsActive)
        {
            EndWave();
        }

        // Calculate result
        CalculateResult();
        m_Result.bSuccess = bSuccess;

        SetState(bSuccess ? EMissionState::Complete : EMissionState::Failed);

        // Notify callbacks
        OnMissionEvent(bSuccess ? "MissionComplete" : "MissionFailed", &m_Result);
    }

    void FMissionManager::AbortMission()
    {
        USS_LOG("Aborting mission");

        if (m_WaveInfo.bIsActive)
        {
            EndWave();
        }

        m_Result = FMissionResult();
        m_Result.bSuccess = false;

        SetState(EMissionState::None);

        OnMissionEvent("MissionAborted", nullptr);
    }

    void FMissionManager::AddObjective(std::unique_ptr<FMissionObjective> Objective)
    {
        if (Objective)
        {
            USS_LOG("Adding objective: %s", Objective->GetDisplayText().c_str());
            m_Objectives.push_back(std::move(Objective));
        }
    }

    FMissionObjective* FMissionManager::GetObjective(int32 Index) const
    {
        if (Index >= 0 && Index < static_cast<int32>(m_Objectives.size()))
        {
            return m_Objectives[Index].get();
        }
        return nullptr;
    }

    void FMissionManager::UpdateObjectiveProgress(int32 Index, int32 Progress)
    {
        FMissionObjective* Objective = GetObjective(Index);
        if (Objective)
        {
            Objective->SetProgress(Progress);
        }
    }

    void FMissionManager::StartWave(int32 WaveNumber)
    {
        if (m_State != EMissionState::Active && m_State != EMissionState::DefensePhase)
        {
            USS_WARN("Cannot start wave - mission not active");
            return;
        }

        USS_LOG("Starting wave %d/%d", WaveNumber, m_WaveInfo.MaxWaves);

        m_WaveInfo.CurrentWave = WaveNumber;
        m_WaveInfo.bIsActive = true;
        m_WaveInfo.EnemiesSpawned = 0;
        m_WaveInfo.EnemiesRemaining = 0;
        m_WaveInfo.WaveStartTime = 0.0f;  // Would get from engine

        SetState(EMissionState::DefensePhase);

        OnMissionEvent("WaveStarted", &m_WaveInfo);
    }

    void FMissionManager::EndWave()
    {
        if (!m_WaveInfo.bIsActive)
            return;

        USS_LOG("Ending wave %d", m_WaveInfo.CurrentWave);

        m_WaveInfo.bIsActive = false;

        OnMissionEvent("WaveEnded", &m_WaveInfo);

        // Check if all waves complete
        if (m_WaveInfo.CurrentWave >= m_WaveInfo.MaxWaves)
        {
            USS_LOG("All waves complete");
            OnMissionEvent("AllWavesComplete", nullptr);
        }
        else
        {
            // Return to active state between waves
            SetState(EMissionState::Active);
        }
    }

    void FMissionManager::AddScore(int32 Points)
    {
        m_Score += Points;
        USS_LOG("Score: %d (+%d)", m_Score, Points);
    }

    void FMissionManager::OnProcessEvent(void* Object, void* Function, void* Params)
    {
        // Route relevant ProcessEvent calls to mission system
        // TODO: Would check function name and dispatch accordingly
		// Prefferrably we would avoid PE alltogether and hook into specific game events

        // Example event routing:
        // "ServerHandleEnemyKilled" -> OnMissionEvent("EnemyKilled", Params)
        // "ServerHandleSurvivorRescued" -> OnMissionEvent("SurvivorRescued", Params)
    }

    void FMissionManager::OnMissionEvent(const char* EventName, void* Params)
    {
        // Dispatch to objectives
        for (auto& Objective : m_Objectives)
        {
            if (Objective)
            {
                Objective->OnEvent(EventName, Params);
            }
        }

        // Dispatch to registered callbacks
        for (const auto& Callback : m_EventCallbacks)
        {
            if (Callback)
            {
                Callback(EventName, Params);
            }
        }
    }

    void FMissionManager::RegisterEventCallback(FMissionEventCallback Callback)
    {
        if (Callback)
        {
            m_EventCallbacks.push_back(std::move(Callback));
        }
    }

    FMissionResult FMissionManager::GetResult() const
    {
        return m_Result;
    }

    void FMissionManager::SetState(EMissionState NewState)
    {
        if (m_State != NewState)
        {
            EMissionState OldState = m_State;
            m_State = NewState;

            USS_LOG("Mission state: %d -> %d", static_cast<int>(OldState), static_cast<int>(NewState));
        }
    }

    void FMissionManager::CreateDefaultObjectives()
    {
        // Create objectives based on mission type
        switch (m_Config.Type)
        {
        case EMissionType::FarmsteadDefense:
        {
            FObjectiveDefinition DefendDef;
            DefendDef.Type = EObjectiveType::Defend;
            DefendDef.DisplayText = "Defend the Atlas";
            DefendDef.TimeLimit = 480.0f;  // 8 minutes
            DefendDef.bIsPrimary = true;
            AddObjective(CreateObjective(DefendDef));
            break;
        }

        case EMissionType::SurvivorsRescue:
        {
            FObjectiveDefinition RescueDef;
            RescueDef.Type = EObjectiveType::Rescue;
            RescueDef.DisplayText = "Rescue Survivors";
            RescueDef.TargetCount = 6;
            RescueDef.bIsPrimary = true;
            AddObjective(CreateObjective(RescueDef));
            break;
        }

        case EMissionType::EncampmentDestruction:
        {
            FObjectiveDefinition DestroyDef;
            DestroyDef.Type = EObjectiveType::Kill;
            DestroyDef.DisplayText = "Destroy Encampments";
            DestroyDef.TargetCount = 5;
            DestroyDef.bIsPrimary = true;
            AddObjective(CreateObjective(DestroyDef));
            break;
        }

        case EMissionType::StormShieldDefense:
        {
            FObjectiveDefinition DefendDef;
            DefendDef.Type = EObjectiveType::Defend;
            DefendDef.DisplayText = "Defend the Storm Shield";
            DefendDef.bIsPrimary = true;
            AddObjective(CreateObjective(DefendDef));
            break;
        }

        case EMissionType::RadarGridConstruction:
        {
            FObjectiveDefinition BuildDef;
            BuildDef.Type = EObjectiveType::Build;
            BuildDef.DisplayText = "Build Radar Towers";
            BuildDef.TargetCount = 5;
            BuildDef.bIsPrimary = true;
            AddObjective(CreateObjective(BuildDef));
            break;
        }

        default:
        {
            // Generic kill objective
            FObjectiveDefinition KillDef;
            KillDef.Type = EObjectiveType::Kill;
            KillDef.DisplayText = "Eliminate Enemies";
            KillDef.TargetCount = 50;
            KillDef.bIsPrimary = true;
            AddObjective(CreateObjective(KillDef));
            break;
        }
        }
    }

    void FMissionManager::CheckObjectivesComplete()
    {
        bool bAllPrimaryComplete = true;
        bool bAnyPrimaryFailed = false;

        for (const auto& Objective : m_Objectives)
        {
            if (Objective && Objective->GetDefinition().bIsPrimary)
            {
                if (Objective->IsFailed())
                {
                    bAnyPrimaryFailed = true;
                    break;
                }

                if (!Objective->IsComplete())
                {
                    bAllPrimaryComplete = false;
                }
            }
        }

        if (bAnyPrimaryFailed)
        {
            EndMission(false);
        }
        else if (bAllPrimaryComplete)
        {
            EndMission(true);
        }
    }

    void FMissionManager::CalculateResult()
    {
        m_Result.FinalScore = m_Score;
        m_Result.WavesCompleted = m_WaveInfo.CurrentWave;
        m_Result.TotalWaves = m_WaveInfo.MaxWaves;
        m_Result.bSuccess = true;

        // Count objective completions
        m_Result.ObjectivesCompleted = 0;
        m_Result.TotalObjectives = static_cast<int32>(m_Objectives.size());

        for (const auto& Objective : m_Objectives)
        {
            if (Objective && Objective->IsComplete())
            {
                m_Result.ObjectivesCompleted++;
            }
            else if (Objective && Objective->IsFailed() && Objective->GetDefinition().bIsPrimary)
            {
                m_Result.bSuccess = false;
            }
        }

        USS_LOG("Mission result: Score=%d, Objectives=%d/%d, Waves=%d/%d",
            m_Result.FinalScore,
            m_Result.ObjectivesCompleted,
            m_Result.TotalObjectives,
            m_Result.WavesCompleted,
            m_Result.TotalWaves);
    }

}