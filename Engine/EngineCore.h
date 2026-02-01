/**
 * UniversalSlashingSimulator - Engine Core
 *
 * Central initialization and management of all engine abstractions.
 * Provides unified access to version resolver, object array, name pool,
 * and other core engine systems.
 */

#pragma once

#include "../Core/Common.h"
#include "../Core/Versioning/VersionInfo.h"
#include "CoreTypes/ObjectArray.h"
#include "CoreTypes/NamePool.h"
#include "CoreTypes/OffsetResolver.h"
#include "UObject/UObjectWrapper.h"
#include <string>

namespace USS
{
    // Engine core initialization status
    struct FEngineCoreStatus
    {
        bool bVersionResolved;
        bool bOffsetsResolved;
        bool bObjectArrayInitialized;
        bool bNamePoolInitialized;
        bool bHooksInitialized;
        bool bFullyInitialized;

        FEngineCoreStatus()
            : bVersionResolved(false)
            , bOffsetsResolved(false)
            , bObjectArrayInitialized(false)
            , bNamePoolInitialized(false)
            , bHooksInitialized(false)
            , bFullyInitialized(false)
        {}
    };

    // Engine core manager
    class FEngineCore
    {
    public:
        USS_NON_COPYABLE(FEngineCore)
        USS_NON_MOVABLE(FEngineCore)

        // Get singleton instance
        static FEngineCore& Get();

        // Initialize engine core (all subsystems)
        EResult Initialize();

        // Shutdown engine core
        void Shutdown();

        // Get initialization status
        const FEngineCoreStatus& GetStatus() const { return m_Status; }

        // Check if fully initialized
        bool IsInitialized() const { return m_Status.bFullyInitialized; }

        // Access to core systems
        const FVersionInfo& GetVersionInfo() const;
        const FOffsetTable& GetOffsets() const;
        IObjectArray* GetObjectArray() const { return m_pObjectArray.get(); }
        INamePool* GetNamePool() const { return m_pNamePool.get(); }

        // Object lookup utilities
        UObjectWrapper FindObject(const char* FullName) const;
        UObjectWrapper FindObjectByName(const char* Name) const;
        UClassWrapper FindClass(const char* ClassName) const;

        // Player/World utilities
        void* FindLocalPlayerController() const;
        void* GetWorld() const;

        // Object name utilities
        std::string GetObjectName(void* Object) const;
        std::string GetObjectClassName(void* Object) const;
        void* GetObjectClass(void* Object) const;
        std::string GetNameFromIndex(int32 NameIndex) const;

        // Offset resolver access
        IOffsetResolver& GetOffsetResolver() const;

        // Object iteration
        template<typename Callback>
        void ForEachObject(Callback&& Func) const
        {
            if (!m_pObjectArray)
                return;

            int32 Num = m_pObjectArray->Num();
            for (int32 i = 0; i < Num; ++i)
            {
                void* Obj = m_pObjectArray->GetByIndex(i);
                if (Obj)
                {
                    if (!Func(UObjectWrapper(Obj)))
                        break;  // Callback returned false, stop iteration
                }
            }
        }

    private:
        FEngineCore();
        ~FEngineCore();

        // Initialization steps
        EResult InitializeVersion();
        EResult InitializeOffsets();
        EResult InitializeObjectArray();
        EResult InitializeNamePool();
        EResult InitializeHooks();

        // Find addresses via patterns
        uintptr FindGObjectsAddress();
        uintptr FindGNamesAddress();
        uintptr FindGWorldAddress();

        FEngineCoreStatus m_Status;

        std::unique_ptr<IObjectArray> m_pObjectArray;
        std::unique_ptr<INamePool> m_pNamePool;

        uintptr m_GObjectsAddress;
        uintptr m_GNamesAddress;
        uintptr m_GWorldAddress;
    };

    // Convenience function
    inline FEngineCore& GetEngineCore()
    {
        return FEngineCore::Get();
    }

}