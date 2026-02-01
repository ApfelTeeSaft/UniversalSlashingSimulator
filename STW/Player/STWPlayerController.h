/**
 * UniversalSlashingSimulator - STW Player Controller
 *
 * Wrapper for AFortPlayerController with STW-specific functionality.
 * Handles player input, abilities, and communication with server.
 */

#pragma once

#include "../../Core/Common.h"
#include "../../Engine/UObject/UObjectWrapper.h"
#include <string>
#include <vector>

namespace USS
{
    // Forward declarations
    class FSTWPlayerPawn;
    class FInventoryManager;

    // Player state for STW
    enum class EPlayerReadyState : uint8
    {
        NotReady,
        Ready,
        InGame,
        Spectating
    };

    // Player info structure
    struct FSTWPlayerInfo
    {
        std::string PlayerName;
        std::string PlayerId;          // EpicID/AccountID
        int32 TeamIndex;
        int32 SquadSlot;
        int32 PowerLevel;
        int32 CommanderLevel;
        EPlayerReadyState ReadyState;
        bool bIsPartyLeader;
        bool bIsValid;

        FSTWPlayerInfo()
            : TeamIndex(0)
            , SquadSlot(-1)
            , PowerLevel(1)
            , CommanderLevel(1)
            , ReadyState(EPlayerReadyState::NotReady)
            , bIsPartyLeader(false)
            , bIsValid(false)
        {}
    };

    // STW Player Controller wrapper
    class FSTWPlayerController
    {
    public:
        FSTWPlayerController();
        explicit FSTWPlayerController(void* InController);
        ~FSTWPlayerController();

        USS_NON_COPYABLE(FSTWPlayerController)

        // Validity
        bool IsValid() const;
        void* GetNative() const { return m_Controller.GetRaw(); }

        // Player info
        FSTWPlayerInfo GetPlayerInfo() const;
        std::string GetPlayerName() const;
        std::string GetPlayerId() const;

        // Pawn management
        FSTWPlayerPawn* GetPawn() const;
        void SetPawn(void* InPawn);
        bool HasPawn() const;

        // State
        EPlayerReadyState GetReadyState() const { return m_ReadyState; }
        void SetReadyState(EPlayerReadyState State);
        bool IsReady() const { return m_ReadyState == EPlayerReadyState::Ready; }
        bool IsInGame() const { return m_ReadyState == EPlayerReadyState::InGame; }

        // Team
        int32 GetTeamIndex() const { return m_TeamIndex; }
        int32 GetSquadSlot() const { return m_SquadSlot; }
        void SetTeamInfo(int32 TeamIndex, int32 SquadSlot);

        // Abilities (STW heroes have abilities)
        void ActivateAbility(int32 AbilityIndex);
        void DeactivateAbility(int32 AbilityIndex);
        bool IsAbilityReady(int32 AbilityIndex) const;

        // Gadgets
        void UseGadget(int32 GadgetSlot);
        bool IsGadgetReady(int32 GadgetSlot) const;

        // Building mode
        void EnterBuildMode();
        void ExitBuildMode();
        bool IsInBuildMode() const { return m_bInBuildMode; }

        // Communication
        void SendChatMessage(const char* Message);
        void ShowNotification(const char* Message, float Duration = 3.0f);

        // Server RPCs (stubs - actual implementation calls engine functions)
        void ServerAcknowledgePossession(void* Pawn);
        void ServerSetReadyState(EPlayerReadyState State);
        void ServerRequestRespawn();

        // Event handling
        void OnProcessEvent(void* Function, void* Params);

    private:
        void UpdateFromNative();

        UObjectWrapper m_Controller;  // AFortPlayerController*
        std::unique_ptr<FSTWPlayerPawn> m_pPawn;

        // Cached state
        EPlayerReadyState m_ReadyState;
        int32 m_TeamIndex;
        int32 m_SquadSlot;
        bool m_bInBuildMode;

        // Cached player info
        mutable FSTWPlayerInfo m_CachedInfo;
        mutable bool m_bInfoDirty;
    };

    // Player controller manager - tracks all connected players
    class FPlayerControllerManager
    {
    public:
        static FPlayerControllerManager& Get();

        USS_NON_COPYABLE(FPlayerControllerManager)
        USS_NON_MOVABLE(FPlayerControllerManager)

        // Lifecycle
        EResult Initialize();
        void Shutdown();
        void Update();

        // Player management
        FSTWPlayerController* RegisterPlayer(void* Controller);
        void UnregisterPlayer(void* Controller);
        FSTWPlayerController* GetPlayer(void* Controller) const;
        FSTWPlayerController* GetPlayerById(const char* PlayerId) const;

        // Iteration
        int32 GetPlayerCount() const { return static_cast<int32>(m_Players.size()); }
        FSTWPlayerController* GetPlayerByIndex(int32 Index) const;

        template<typename Callback>
        void ForEachPlayer(Callback&& Func) const
        {
            for (const auto& Pair : m_Players)
            {
                if (Pair.second)
                {
                    if (!Func(Pair.second.get()))
                        break;
                }
            }
        }

        // Team queries
        std::vector<FSTWPlayerController*> GetPlayersOnTeam(int32 TeamIndex) const;
        int32 GetReadyPlayerCount() const;
        bool AreAllPlayersReady() const;

        // Events
        void OnPlayerJoined(void* Controller);
        void OnPlayerLeft(void* Controller);

    private:
        FPlayerControllerManager() = default;
        ~FPlayerControllerManager() = default;

        std::unordered_map<void*, std::unique_ptr<FSTWPlayerController>> m_Players;
    };

    inline FPlayerControllerManager& GetPlayerControllerManager()
    {
        return FPlayerControllerManager::Get();
    }

}