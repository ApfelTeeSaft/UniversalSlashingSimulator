/**
 * UniversalSlashingSimulator - Hook Types
 *
 * Hooking system using MinHook for function detouring.
 * Requires MinHook library to be linked.
 */

#pragma once

#include "../Common.h"
#include "../Logging/Log.h"
#include <MinHook.h>
#include <functional>
#include <vector>

namespace USS
{
    using ProcessEventFn = void(*)(void* Object, void* Function, void* Params);

    namespace Hook
    {
        namespace Detail
        {
            inline bool g_bInitialized = false;
            inline FCriticalSection g_CriticalSection;
        }

        /**
         * Initialize the hook library (MinHook)
         * Must be called before any other Hook functions
         */
        inline EResult Initialize()
        {
            FScopedLock Lock(Detail::g_CriticalSection);

            if (Detail::g_bInitialized)
                return EResult::AlreadyInitialized;

            MH_STATUS Status = MH_Initialize();
            if (Status != MH_OK)
            {
                USS_ERROR("MinHook initialization failed: %s", MH_StatusToString(Status));
                return EResult::Failed;
            }

            Detail::g_bInitialized = true;
            USS_LOG("MinHook initialized successfully");
            return EResult::Success;
        }

        /**
         * Shutdown the hook library
         * Removes all hooks and cleans up
         */
        inline void Shutdown()
        {
            FScopedLock Lock(Detail::g_CriticalSection);

            if (!Detail::g_bInitialized)
                return;

            MH_DisableHook(MH_ALL_HOOKS);

            MH_STATUS Status = MH_Uninitialize();
            if (Status != MH_OK)
            {
                USS_WARN("MinHook shutdown warning: %s", MH_StatusToString(Status));
            }
            else
            {
                USS_LOG("MinHook shutdown successfully");
            }

            Detail::g_bInitialized = false;
        }

        /**
         * Check if hook system is initialized
         */
        inline bool IsInitialized()
        {
            return Detail::g_bInitialized;
        }

        /**
         * Create and enable a hook in one call
         * @param Target - Address of function to hook
         * @param Detour - Your detour function
         * @param OutOriginal - Receives pointer to original function (trampoline)
         * @return EResult::Success on success
         */
        template<typename T>
        inline EResult CreateAndEnable(uintptr Target, T Detour, T* OutOriginal)
        {
            if (!Detail::g_bInitialized)
            {
                USS_ERROR("Hook::CreateAndEnable called before Initialize");
                return EResult::NotInitialized;
            }

            if (Target == 0 || Detour == nullptr)
            {
                USS_ERROR("Hook::CreateAndEnable - invalid parameters");
                return EResult::InvalidParameter;
            }

            void* pTarget = reinterpret_cast<void*>(Target);
            void* pDetour = reinterpret_cast<void*>(Detour);
            void** ppOriginal = reinterpret_cast<void**>(OutOriginal);

            MH_STATUS Status = MH_CreateHook(pTarget, pDetour, ppOriginal);
            if (Status != MH_OK)
            {
                USS_ERROR("MH_CreateHook failed at 0x%llX: %s", Target, MH_StatusToString(Status));
                return EResult::HookFailed;
            }

            Status = MH_EnableHook(pTarget);
            if (Status != MH_OK)
            {
                USS_ERROR("MH_EnableHook failed at 0x%llX: %s", Target, MH_StatusToString(Status));
                MH_RemoveHook(pTarget);
                return EResult::HookFailed;
            }

            USS_LOG("Hook created and enabled at 0x%llX", Target);
            return EResult::Success;
        }

        /**
         * Create a hook without enabling it
         */
        template<typename T>
        inline EResult Create(uintptr Target, T Detour, T* OutOriginal)
        {
            if (!Detail::g_bInitialized)
                return EResult::NotInitialized;

            void* pTarget = reinterpret_cast<void*>(Target);
            void* pDetour = reinterpret_cast<void*>(Detour);
            void** ppOriginal = reinterpret_cast<void**>(OutOriginal);

            MH_STATUS Status = MH_CreateHook(pTarget, pDetour, ppOriginal);
            if (Status != MH_OK)
            {
                USS_ERROR("MH_CreateHook failed at 0x%llX: %s", Target, MH_StatusToString(Status));
                return EResult::HookFailed;
            }

            USS_LOG("Hook created (disabled) at 0x%llX", Target);
            return EResult::Success;
        }

        /**
         * Enable a previously created hook
         */
        inline EResult Enable(uintptr Target)
        {
            if (!Detail::g_bInitialized)
                return EResult::NotInitialized;

            void* pTarget = reinterpret_cast<void*>(Target);
            MH_STATUS Status = MH_EnableHook(pTarget);

            if (Status != MH_OK)
            {
                USS_ERROR("MH_EnableHook failed at 0x%llX: %s", Target, MH_StatusToString(Status));
                return EResult::HookFailed;
            }

            return EResult::Success;
        }

        /**
         * Disable a hook (can be re-enabled later)
         */
        inline EResult Disable(uintptr Target)
        {
            if (!Detail::g_bInitialized)
                return EResult::NotInitialized;

            void* pTarget = reinterpret_cast<void*>(Target);
            MH_STATUS Status = MH_DisableHook(pTarget);

            if (Status != MH_OK)
            {
                USS_ERROR("MH_DisableHook failed at 0x%llX: %s", Target, MH_StatusToString(Status));
                return EResult::HookFailed;
            }

            return EResult::Success;
        }

        /**
         * Remove a hook completely
         */
        inline EResult Remove(uintptr Target)
        {
            if (!Detail::g_bInitialized)
                return EResult::NotInitialized;

            void* pTarget = reinterpret_cast<void*>(Target);

            MH_DisableHook(pTarget);

            MH_STATUS Status = MH_RemoveHook(pTarget);
            if (Status != MH_OK)
            {
                USS_ERROR("MH_RemoveHook failed at 0x%llX: %s", Target, MH_StatusToString(Status));
                return EResult::HookFailed;
            }

            USS_LOG("Hook removed at 0x%llX", Target);
            return EResult::Success;
        }

        /**
         * Enable all hooks
         */
        inline EResult EnableAll()
        {
            if (!Detail::g_bInitialized)
                return EResult::NotInitialized;

            MH_STATUS Status = MH_EnableHook(MH_ALL_HOOKS);
            return (Status == MH_OK) ? EResult::Success : EResult::HookFailed;
        }

        /**
         * Disable all hooks
         */
        inline EResult DisableAll()
        {
            if (!Detail::g_bInitialized)
                return EResult::NotInitialized;

            MH_STATUS Status = MH_DisableHook(MH_ALL_HOOKS);
            return (Status == MH_OK) ? EResult::Success : EResult::HookFailed;
        }

        /**
         * Queue a hook to be enabled (for batch enabling)
         */
        inline EResult QueueEnable(uintptr Target)
        {
            if (!Detail::g_bInitialized)
                return EResult::NotInitialized;

            void* pTarget = reinterpret_cast<void*>(Target);
            MH_STATUS Status = MH_QueueEnableHook(pTarget);
            return (Status == MH_OK) ? EResult::Success : EResult::HookFailed;
        }

        /**
         * Queue a hook to be disabled (for batch disabling)
         */
        inline EResult QueueDisable(uintptr Target)
        {
            if (!Detail::g_bInitialized)
                return EResult::NotInitialized;

            void* pTarget = reinterpret_cast<void*>(Target);
            MH_STATUS Status = MH_QueueDisableHook(pTarget);
            return (Status == MH_OK) ? EResult::Success : EResult::HookFailed;
        }

        /**
         * Apply all queued enable/disable operations
         */
        inline EResult ApplyQueued()
        {
            if (!Detail::g_bInitialized)
                return EResult::NotInitialized;

            MH_STATUS Status = MH_ApplyQueued();
            return (Status == MH_OK) ? EResult::Success : EResult::HookFailed;
        }
    }

    // ========================================================================
    // ProcessEvent Dispatcher
    // ========================================================================

    /**
     * Simple ProcessEvent hook manager
     * Manages callbacks for ProcessEvent interception
     */
    class FSimpleProcessEventDispatcher
    {
    public:
        using PreCallback = std::function<bool(void* Object, void* Function, void* Params)>;
        using PostCallback = std::function<void(void* Object, void* Function, void* Params)>;

        static FSimpleProcessEventDispatcher& Get()
        {
            static FSimpleProcessEventDispatcher Instance;
            return Instance;
        }

        void RegisterPre(PreCallback Callback)
        {
            FScopedLock Lock(m_CriticalSection);
            m_PreCallbacks.push_back(std::move(Callback));
        }

        void RegisterPost(PostCallback Callback)
        {
            FScopedLock Lock(m_CriticalSection);
            m_PostCallbacks.push_back(std::move(Callback));
        }

        bool DispatchPre(void* Object, void* Function, void* Params)
        {
            bool bCallOriginal = true;

            std::vector<PreCallback> Callbacks;
            {
                FScopedLock Lock(m_CriticalSection);
                Callbacks = m_PreCallbacks;
            }

            for (const auto& Cb : Callbacks)
            {
                if (!Cb(Object, Function, Params))
                    bCallOriginal = false;
            }
            return bCallOriginal;
        }

        void DispatchPost(void* Object, void* Function, void* Params)
        {
            std::vector<PostCallback> Callbacks;
            {
                FScopedLock Lock(m_CriticalSection);
                Callbacks = m_PostCallbacks;
            }

            for (const auto& Cb : Callbacks)
            {
                Cb(Object, Function, Params);
            }
        }

        void SetOriginal(ProcessEventFn Original) { m_pOriginal = Original; }
        ProcessEventFn GetOriginal() const { return m_pOriginal; }

        void ClearCallbacks()
        {
            FScopedLock Lock(m_CriticalSection);
            m_PreCallbacks.clear();
            m_PostCallbacks.clear();
        }

    private:
        FSimpleProcessEventDispatcher() : m_pOriginal(nullptr) {}

        FCriticalSection m_CriticalSection;
        std::vector<PreCallback> m_PreCallbacks;
        std::vector<PostCallback> m_PostCallbacks;
        ProcessEventFn m_pOriginal;
    };

    inline FSimpleProcessEventDispatcher& GetSimpleProcessEventDispatcher()
    {
        return FSimpleProcessEventDispatcher::Get();
    }

}