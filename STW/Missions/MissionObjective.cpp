/**
 * UniversalSlashingSimulator - Mission Objective Implementation
 */

#include "MissionObjective.h"
#include "../../Core/Logging/Log.h"

namespace USS
{
    // ========================================================================
    // FMissionObjective (Base)
    // ========================================================================

    FMissionObjective::FMissionObjective(const FObjectiveDefinition& Definition)
        : m_Definition(Definition)
        , m_State(EObjectiveState::Inactive)
        , m_CurrentProgress(0)
    {
    }

    void FMissionObjective::Update()
    {
        // Base implementation does nothing
        // Derived classes override for time-based updates
    }

    void FMissionObjective::OnEvent(const char* EventName, void* Params)
    {
        // Base implementation does nothing
        // Derived classes handle specific events
    }

    float FMissionObjective::GetProgressPercent() const
    {
        if (m_Definition.TargetCount <= 0)
            return m_State == EObjectiveState::Completed ? 1.0f : 0.0f;

        return static_cast<float>(m_CurrentProgress) / static_cast<float>(m_Definition.TargetCount);
    }

    void FMissionObjective::AddProgress(int32 Amount)
    {
        SetProgress(m_CurrentProgress + Amount);
    }

    void FMissionObjective::SetProgress(int32 Progress)
    {
        int32 OldProgress = m_CurrentProgress;
        m_CurrentProgress = Progress;

        if (m_CurrentProgress < 0)
            m_CurrentProgress = 0;

        if (m_CurrentProgress != OldProgress)
        {
            OnProgressChanged();

            // Auto-complete if target reached
            if (m_Definition.TargetCount > 0 && m_CurrentProgress >= m_Definition.TargetCount)
            {
                Complete();
            }
        }
    }

    void FMissionObjective::Complete()
    {
        if (m_State != EObjectiveState::Completed && m_State != EObjectiveState::Failed)
        {
            SetState(EObjectiveState::Completed);
            USS_LOG("Objective completed: %s", m_Definition.DisplayText.c_str());
        }
    }

    void FMissionObjective::Fail()
    {
        if (m_State != EObjectiveState::Completed && m_State != EObjectiveState::Failed)
        {
            SetState(EObjectiveState::Failed);
            USS_LOG("Objective failed: %s", m_Definition.DisplayText.c_str());
        }
    }

    void FMissionObjective::OnProgressChanged()
    {
        USS_LOG("Objective progress: %s - %d/%d",
            m_Definition.DisplayText.c_str(),
            m_CurrentProgress,
            m_Definition.TargetCount);
    }

    void FMissionObjective::OnStateChanged(EObjectiveState OldState)
    {
        // Base implementation does nothing
    }

    void FMissionObjective::SetState(EObjectiveState NewState)
    {
        if (m_State != NewState)
        {
            EObjectiveState OldState = m_State;
            m_State = NewState;
            OnStateChanged(OldState);
        }
    }

    // ========================================================================
    // FKillObjective
    // ========================================================================

    FKillObjective::FKillObjective(const FObjectiveDefinition& Definition)
        : FMissionObjective(Definition)
    {
        // Could parse target class from definition metadata
    }

    void FKillObjective::OnEvent(const char* EventName, void* Params)
    {
        if (m_State != EObjectiveState::Active)
            return;

        // Handle enemy killed event
        // TODO: Check if EventName matches kill event,
        // validate killed actor class, increment progress
        if (strcmp(EventName, "EnemyKilled") == 0)
        {
            // TODO: Validate target class if specified
            AddProgress(1);
        }
    }

    // ========================================================================
    // FCollectObjective
    // ========================================================================

    FCollectObjective::FCollectObjective(const FObjectiveDefinition& Definition)
        : FMissionObjective(Definition)
    {
    }

    void FCollectObjective::OnEvent(const char* EventName, void* Params)
    {
        if (m_State != EObjectiveState::Active)
            return;

        if (strcmp(EventName, "ItemCollected") == 0)
        {
            // TODO: Validate item class
            AddProgress(1);
        }
    }

    // ========================================================================
    // FDefendObjective
    // ========================================================================

    FDefendObjective::FDefendObjective(const FObjectiveDefinition& Definition)
        : FMissionObjective(Definition)
        , m_DefendTimeRemaining(Definition.TimeLimit)
        , m_CurrentHealth(100.0f)
        , m_MaxHealth(100.0f)
        , m_bDefenseActive(false)
    {
    }

    void FDefendObjective::Update()
    {
        if (m_State != EObjectiveState::Active || !m_bDefenseActive)
            return;

        // TODO: Get delta time from engine
        float DeltaTime = 1.0f / 30.0f;  // Placeholder 30fps

        m_DefendTimeRemaining -= DeltaTime;

        if (m_DefendTimeRemaining <= 0.0f)
        {
            m_DefendTimeRemaining = 0.0f;
            Complete();
        }
    }

    void FDefendObjective::OnEvent(const char* EventName, void* Params)
    {
        if (m_State != EObjectiveState::Active)
            return;

        if (strcmp(EventName, "DefenseStarted") == 0)
        {
            m_bDefenseActive = true;
            USS_LOG("Defense phase started");
        }
        else if (strcmp(EventName, "ObjectDamaged") == 0)
        {
            // TODO: Extract damage amount from Params
            float Damage = 10.0f;  // Placeholder
            m_CurrentHealth -= Damage;

            if (m_CurrentHealth <= 0.0f)
            {
                m_CurrentHealth = 0.0f;
                Fail();
            }
        }
    }

    float FDefendObjective::GetHealthPercent() const
    {
        if (m_MaxHealth <= 0.0f)
            return 1.0f;
        return m_CurrentHealth / m_MaxHealth;
    }

    // ========================================================================
    // FExploreObjective
    // ========================================================================

    FExploreObjective::FExploreObjective(const FObjectiveDefinition& Definition)
        : FMissionObjective(Definition)
        , m_bLocationReached(false)
    {
    }

    void FExploreObjective::OnEvent(const char* EventName, void* Params)
    {
        if (m_State != EObjectiveState::Active)
            return;

        if (strcmp(EventName, "LocationReached") == 0)
        {
            // TODO: Validate location matches objective
            m_bLocationReached = true;
            Complete();
        }
    }

    // ========================================================================
    // FBuildObjective
    // ========================================================================

    FBuildObjective::FBuildObjective(const FObjectiveDefinition& Definition)
        : FMissionObjective(Definition)
    {
    }

    void FBuildObjective::OnEvent(const char* EventName, void* Params)
    {
        if (m_State != EObjectiveState::Active)
            return;

        if (strcmp(EventName, "BuildingPlaced") == 0)
        {
            // TODO: Validate building class if specified
            AddProgress(1);
        }
    }

    // ========================================================================
    // FRescueObjective
    // ========================================================================

    FRescueObjective::FRescueObjective(const FObjectiveDefinition& Definition)
        : FMissionObjective(Definition)
        , m_SurvivorsLost(0)
        , m_MaxLosses(0)  // Could be set from definition
    {
    }

    void FRescueObjective::OnEvent(const char* EventName, void* Params)
    {
        if (m_State != EObjectiveState::Active)
            return;

        if (strcmp(EventName, "SurvivorRescued") == 0)
        {
            AddProgress(1);
        }
        else if (strcmp(EventName, "SurvivorLost") == 0)
        {
            m_SurvivorsLost++;

            if (m_MaxLosses > 0 && m_SurvivorsLost >= m_MaxLosses)
            {
                Fail();
            }
        }
    }

    // ========================================================================
    // Factory
    // ========================================================================

    std::unique_ptr<FMissionObjective> CreateObjective(const FObjectiveDefinition& Definition)
    {
        switch (Definition.Type)
        {
        case EObjectiveType::Kill:
            return std::make_unique<FKillObjective>(Definition);

        case EObjectiveType::Collect:
            return std::make_unique<FCollectObjective>(Definition);

        case EObjectiveType::Defend:
            return std::make_unique<FDefendObjective>(Definition);

        case EObjectiveType::Explore:
            return std::make_unique<FExploreObjective>(Definition);

        case EObjectiveType::Build:
            return std::make_unique<FBuildObjective>(Definition);

        case EObjectiveType::Rescue:
            return std::make_unique<FRescueObjective>(Definition);

        default:
            USS_WARN("Unknown objective type: %d", static_cast<int>(Definition.Type));
            return std::make_unique<FMissionObjective>(Definition);
        }
    }

}