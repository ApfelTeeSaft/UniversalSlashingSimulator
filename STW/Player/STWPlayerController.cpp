/**
 * UniversalSlashingSimulator - STW Player Controller Implementation
 */

#include "STWPlayerController.h"
#include "STWPlayerPawn.h"
#include "../../Core/Logging/Log.h"
#include "../../Engine/EngineCore.h"

namespace USS
{
    // ========================================================================
    // FSTWPlayerController
    // ========================================================================

    FSTWPlayerController::FSTWPlayerController()
        : m_ReadyState(EPlayerReadyState::NotReady)
        , m_TeamIndex(0)
        , m_SquadSlot(-1)
        , m_bInBuildMode(false)
        , m_bInfoDirty(true)
    {
    }

    FSTWPlayerController::FSTWPlayerController(void* InController)
        : m_Controller(InController)
        , m_ReadyState(EPlayerReadyState::NotReady)
        , m_TeamIndex(0)
        , m_SquadSlot(-1)
        , m_bInBuildMode(false)
        , m_bInfoDirty(true)
    {
        if (IsValid())
        {
            UpdateFromNative();
        }
    }

    FSTWPlayerController::~FSTWPlayerController()
    {
        m_pPawn.reset();
    }

    bool FSTWPlayerController::IsValid() const
    {
        return m_Controller.IsValid();
    }

    FSTWPlayerInfo FSTWPlayerController::GetPlayerInfo() const
    {
        if (m_bInfoDirty && IsValid())
        {
            // Read from native controller
            // TODO: Read PlayerState properties

            m_CachedInfo.bIsValid = true;
            m_CachedInfo.TeamIndex = m_TeamIndex;
            m_CachedInfo.SquadSlot = m_SquadSlot;
            m_CachedInfo.ReadyState = m_ReadyState;

            // TODO: Read from APlayerState
            // m_CachedInfo.PlayerName = ...
            // m_CachedInfo.PlayerId = ...

            m_bInfoDirty = false;
        }

        return m_CachedInfo;
    }

    std::string FSTWPlayerController::GetPlayerName() const
    {
        if (!IsValid())
            return "";

        // TODO: Read from PlayerState->GetPlayerName()
        return m_CachedInfo.PlayerName;
    }

    std::string FSTWPlayerController::GetPlayerId() const
    {
        if (!IsValid())
            return "";

        // TODO: Read from PlayerState->GetUniqueId()
        return m_CachedInfo.PlayerId;
    }

    FSTWPlayerPawn* FSTWPlayerController::GetPawn() const
    {
        return m_pPawn.get();
    }

    void FSTWPlayerController::SetPawn(void* InPawn)
    {
        if (InPawn)
        {
            m_pPawn = std::make_unique<FSTWPlayerPawn>(InPawn);
            USS_LOG("Pawn set for player: %s", GetPlayerName().c_str());
        }
        else
        {
            m_pPawn.reset();
        }
    }

    bool FSTWPlayerController::HasPawn() const
    {
        return m_pPawn && m_pPawn->IsValid();
    }

    void FSTWPlayerController::SetReadyState(EPlayerReadyState State)
    {
        if (m_ReadyState != State)
        {
            m_ReadyState = State;
            m_bInfoDirty = true;

            USS_LOG("Player %s ready state: %d", GetPlayerName().c_str(), static_cast<int>(State));
        }
    }

    void FSTWPlayerController::SetTeamInfo(int32 TeamIndex, int32 SquadSlot)
    {
        m_TeamIndex = TeamIndex;
        m_SquadSlot = SquadSlot;
        m_bInfoDirty = true;
    }

    void FSTWPlayerController::ActivateAbility(int32 AbilityIndex)
    {
        if (!HasPawn())
            return;

        // TODO: Call ability activation on pawn's AbilitySystemComponent
        USS_LOG("Activating ability %d for player %s", AbilityIndex, GetPlayerName().c_str());
    }

    void FSTWPlayerController::DeactivateAbility(int32 AbilityIndex)
    {
        if (!HasPawn())
            return;

        USS_LOG("Deactivating ability %d", AbilityIndex);
    }

    bool FSTWPlayerController::IsAbilityReady(int32 AbilityIndex) const
    {
        if (!HasPawn())
            return false;

        // TODO: Check ability cooldown
        return true;
    }

    void FSTWPlayerController::UseGadget(int32 GadgetSlot)
    {
        if (!HasPawn())
            return;

        USS_LOG("Using gadget slot %d", GadgetSlot);
        // TODO: Trigger gadget use
    }

    bool FSTWPlayerController::IsGadgetReady(int32 GadgetSlot) const
    {
        // TODO: Check gadget cooldown
        return true;
    }

    void FSTWPlayerController::EnterBuildMode()
    {
        if (!m_bInBuildMode)
        {
            m_bInBuildMode = true;
            USS_LOG("Entering build mode");
            // TODO: Set building mode state on controller
        }
    }

    void FSTWPlayerController::ExitBuildMode()
    {
        if (m_bInBuildMode)
        {
            m_bInBuildMode = false;
            USS_LOG("Exiting build mode");
        }
    }

    void FSTWPlayerController::SendChatMessage(const char* Message)
    {
        if (!IsValid() || !Message)
            return;

        USS_LOG("Chat from %s: %s", GetPlayerName().c_str(), Message);
        // TODO: Broadcast to other players
    }

    void FSTWPlayerController::ShowNotification(const char* Message, float Duration)
    {
        if (!IsValid() || !Message)
            return;

        // TODO: Call client RPC to show notification
        USS_LOG("Notification to %s: %s", GetPlayerName().c_str(), Message);
    }

    void FSTWPlayerController::ServerAcknowledgePossession(void* Pawn)
    {
        SetPawn(Pawn);
    }

    void FSTWPlayerController::ServerSetReadyState(EPlayerReadyState State)
    {
        SetReadyState(State);
    }

    void FSTWPlayerController::ServerRequestRespawn()
    {
        if (!IsValid())
            return;

        USS_LOG("Respawn requested for %s", GetPlayerName().c_str());
        // TODO: Handle respawn logic
    }

    void FSTWPlayerController::OnProcessEvent(void* Function, void* Params)
    {
        // Handle controller-specific events
		// // prefferrably avoid PE where possible
        // TODO: Check function name and dispatch
    }

    void FSTWPlayerController::UpdateFromNative()
    {
        if (!IsValid())
            return;

        // Read current state from native controller
        // TODO: Read team, ready state, etc. from engine
        m_bInfoDirty = true;
    }

    // ========================================================================
    // FPlayerControllerManager
    // ========================================================================

    FPlayerControllerManager& FPlayerControllerManager::Get()
    {
        static FPlayerControllerManager Instance;
        return Instance;
    }

    EResult FPlayerControllerManager::Initialize()
    {
        USS_LOG("Initializing Player Controller Manager...");
        m_Players.clear();
        return EResult::Success;
    }

    void FPlayerControllerManager::Shutdown()
    {
        USS_LOG("Shutting down Player Controller Manager");
        m_Players.clear();
    }

    void FPlayerControllerManager::Update()
    {
        // Update all player controllers
        for (auto& Pair : m_Players)
        {
            if (Pair.second && Pair.second->HasPawn())
            {
                Pair.second->GetPawn()->Update();
            }
        }
    }

    FSTWPlayerController* FPlayerControllerManager::RegisterPlayer(void* Controller)
    {
        if (!Controller)
            return nullptr;

        auto It = m_Players.find(Controller);
        if (It != m_Players.end())
        {
            return It->second.get();
        }

        auto NewController = std::make_unique<FSTWPlayerController>(Controller);
        FSTWPlayerController* Result = NewController.get();
        m_Players[Controller] = std::move(NewController);

        USS_LOG("Registered player controller: %p", Controller);

        return Result;
    }

    void FPlayerControllerManager::UnregisterPlayer(void* Controller)
    {
        auto It = m_Players.find(Controller);
        if (It != m_Players.end())
        {
            USS_LOG("Unregistered player controller: %p", Controller);
            m_Players.erase(It);
        }
    }

    FSTWPlayerController* FPlayerControllerManager::GetPlayer(void* Controller) const
    {
        auto It = m_Players.find(Controller);
        return (It != m_Players.end()) ? It->second.get() : nullptr;
    }

    FSTWPlayerController* FPlayerControllerManager::GetPlayerById(const char* PlayerId) const
    {
        if (!PlayerId)
            return nullptr;

        for (const auto& Pair : m_Players)
        {
            if (Pair.second && Pair.second->GetPlayerId() == PlayerId)
            {
                return Pair.second.get();
            }
        }

        return nullptr;
    }

    FSTWPlayerController* FPlayerControllerManager::GetPlayerByIndex(int32 Index) const
    {
        if (Index < 0 || Index >= static_cast<int32>(m_Players.size()))
            return nullptr;

        auto It = m_Players.begin();
        std::advance(It, Index);
        return It->second.get();
    }

    std::vector<FSTWPlayerController*> FPlayerControllerManager::GetPlayersOnTeam(int32 TeamIndex) const
    {
        std::vector<FSTWPlayerController*> Result;

        for (const auto& Pair : m_Players)
        {
            if (Pair.second && Pair.second->GetTeamIndex() == TeamIndex)
            {
                Result.push_back(Pair.second.get());
            }
        }

        return Result;
    }

    int32 FPlayerControllerManager::GetReadyPlayerCount() const
    {
        int32 Count = 0;

        for (const auto& Pair : m_Players)
        {
            if (Pair.second && Pair.second->IsReady())
            {
                Count++;
            }
        }

        return Count;
    }

    bool FPlayerControllerManager::AreAllPlayersReady() const
    {
        if (m_Players.empty())
            return false;

        for (const auto& Pair : m_Players)
        {
            if (Pair.second && !Pair.second->IsReady() && !Pair.second->IsInGame())
            {
                return false;
            }
        }

        return true;
    }

    void FPlayerControllerManager::OnPlayerJoined(void* Controller)
    {
        RegisterPlayer(Controller);
    }

    void FPlayerControllerManager::OnPlayerLeft(void* Controller)
    {
        UnregisterPlayer(Controller);
    }

}