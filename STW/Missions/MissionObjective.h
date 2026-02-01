/**
 * UniversalSlashingSimulator - Mission Objective
 *
 * Base class for mission objectives. Derived classes handle
 * specific objective types (kill, collect, defend, etc.).
 */

#pragma once

#include "../../Core/Common.h"
#include "../../Engine/UObject/UObjectWrapper.h"
#include "MissionTypes.h"
#include <string>

namespace USS
{
    // Base mission objective class
    class FMissionObjective
    {
    public:
        FMissionObjective(const FObjectiveDefinition& Definition);
        virtual ~FMissionObjective() = default;

        USS_NON_COPYABLE(FMissionObjective)

        // Core interface
        virtual void Update();
        virtual void OnEvent(const char* EventName, void* Params);

        // State
        EObjectiveState GetState() const { return m_State; }
        bool IsComplete() const { return m_State == EObjectiveState::Completed; }
        bool IsFailed() const { return m_State == EObjectiveState::Failed; }

        // Progress
        int32 GetCurrentProgress() const { return m_CurrentProgress; }
        int32 GetTargetProgress() const { return m_Definition.TargetCount; }
        float GetProgressPercent() const;

        // Definition
        const FObjectiveDefinition& GetDefinition() const { return m_Definition; }
        EObjectiveType GetType() const { return m_Definition.Type; }
        const std::string& GetDisplayText() const { return m_Definition.DisplayText; }

        // Progress updates
        void AddProgress(int32 Amount);
        void SetProgress(int32 Progress);
        void Complete();
        void Fail();

    protected:
        virtual void OnProgressChanged();
        virtual void OnStateChanged(EObjectiveState OldState);

        void SetState(EObjectiveState NewState);

        FObjectiveDefinition m_Definition;
        EObjectiveState m_State;
        int32 m_CurrentProgress;

        // Cached engine references
        UObjectWrapper m_ObjectiveActor;  // AFortObjective*
    };

    // Kill enemies objective
    class FKillObjective : public FMissionObjective
    {
    public:
        FKillObjective(const FObjectiveDefinition& Definition);

        void OnEvent(const char* EventName, void* Params) override;

    private:
        std::string m_TargetActorClass;  // Specific enemy class to kill, empty = any
    };

    // Collect items objective
    class FCollectObjective : public FMissionObjective
    {
    public:
        FCollectObjective(const FObjectiveDefinition& Definition);

        void OnEvent(const char* EventName, void* Params) override;

    private:
        std::string m_ItemClass;  // Item class to collect
    };

    // Defend location/object objective
    class FDefendObjective : public FMissionObjective
    {
    public:
        FDefendObjective(const FObjectiveDefinition& Definition);

        void Update() override;
        void OnEvent(const char* EventName, void* Params) override;

        // Defend-specific
        float GetDefendTimeRemaining() const { return m_DefendTimeRemaining; }
        float GetDefendTimerTotal() const { return m_Definition.TimeLimit; }
        float GetHealthPercent() const;

    private:
        float m_DefendTimeRemaining;
        float m_CurrentHealth;
        float m_MaxHealth;
        bool m_bDefenseActive;
    };

    // Explore/reach location objective
    class FExploreObjective : public FMissionObjective
    {
    public:
        FExploreObjective(const FObjectiveDefinition& Definition);

        void OnEvent(const char* EventName, void* Params) override;

    private:
        bool m_bLocationReached;
    };

    // Build structures objective
    class FBuildObjective : public FMissionObjective
    {
    public:
        FBuildObjective(const FObjectiveDefinition& Definition);

        void OnEvent(const char* EventName, void* Params) override;

    private:
        std::string m_BuildingClass;  // Specific building type, empty = any
    };

    // Rescue survivors objective
    class FRescueObjective : public FMissionObjective
    {
    public:
        FRescueObjective(const FObjectiveDefinition& Definition);

        void OnEvent(const char* EventName, void* Params) override;

    private:
        int32 m_SurvivorsLost;
        int32 m_MaxLosses;
    };

    // Factory function to create objective by type
    std::unique_ptr<FMissionObjective> CreateObjective(const FObjectiveDefinition& Definition);

}