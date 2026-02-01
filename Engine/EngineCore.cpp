/**
 * UniversalSlashingSimulator - Engine Core Implementation
 */

#include "EngineCore.h"
#include "../Core/Memory/Memory.h"
#include "../Core/Logging/Log.h"
#include "../Core/Versioning/VersionResolver.h"
#include "../Core/Hooks/HookTypes.h"

namespace USS
{
    FEngineCore::FEngineCore()
        : m_GObjectsAddress(0)
        , m_GNamesAddress(0)
        , m_GWorldAddress(0)
    {
    }

    FEngineCore::~FEngineCore()
    {
        Shutdown();
    }

    FEngineCore& FEngineCore::Get()
    {
        static FEngineCore Instance;
        return Instance;
    }

    EResult FEngineCore::Initialize()
    {
        if (m_Status.bFullyInitialized)
            return EResult::AlreadyInitialized;

        USS_LOG("=== UniversalSlashingSimulator Engine Core ===");
        USS_LOG("Initializing engine core...");

        EResult Result = Memory::Initialize();
        if (Result != EResult::Success)
        {
            USS_ERROR("Failed to initialize memory utilities");
            return Result;
        }

        Result = InitializeVersion();
        if (Result != EResult::Success)
        {
            USS_ERROR("Failed to detect version");
            return Result;
        }

        Result = InitializeOffsets();
        if (Result != EResult::Success)
        {
            USS_ERROR("Failed to resolve offsets");
            return Result;
        }

        Result = InitializeObjectArray();
        if (Result != EResult::Success)
        {
            USS_ERROR("Failed to initialize object array");
            return Result;
        }

        Result = InitializeNamePool();
        if (Result != EResult::Success)
        {
            USS_ERROR("Failed to initialize name pool");
            return Result;
        }

        Result = InitializeHooks();
        if (Result != EResult::Success)
        {
            USS_WARN("Hook initialization returned: %s (@timmie must implement)", ResultToString(Result));
            // Non-fatal - hooks are stubbed until @timmie implements
        }

        m_Status.bFullyInitialized = true;
        USS_LOG("Engine core initialized successfully");

        return EResult::Success;
    }

    void FEngineCore::Shutdown()
    {
        if (!m_Status.bFullyInitialized)
            return;

        USS_LOG("Shutting down engine core...");

        Hook::Shutdown();

        m_pNamePool.reset();
        m_pObjectArray.reset();

        m_Status = FEngineCoreStatus();

        USS_LOG("Engine core shutdown complete");
    }

    EResult FEngineCore::InitializeVersion()
    {
        USS_LOG("Detecting engine version...");

        EResult Result = GetVersionResolver().DetectVersion();
        if (Result != EResult::Success)
            return Result;

        m_Status.bVersionResolved = true;
        return EResult::Success;
    }

    EResult FEngineCore::InitializeOffsets()
    {
        USS_LOG("Resolving offsets...");

        const auto& Version = GetVersionResolver().GetVersionInfo();
        EResult Result = FStubOffsetResolver::Get().ResolveOffsets(Version);
        if (Result != EResult::Success)
            return Result;

        m_Status.bOffsetsResolved = true;
        return EResult::Success;
    }

    EResult FEngineCore::InitializeObjectArray()
    {
        USS_LOG("Initializing object array...");

        m_GObjectsAddress = FindGObjectsAddress();
        if (m_GObjectsAddress == 0)
        {
            USS_ERROR("Failed to find GObjects address");
            return EResult::PatternNotFound;
        }

        USS_LOG("GObjects at 0x%llX", m_GObjectsAddress);

        m_pObjectArray = CreateObjectArray();
        if (!m_pObjectArray)
        {
            USS_ERROR("Failed to create object array");
            return EResult::Failed;
        }

        EResult Result = m_pObjectArray->Initialize(m_GObjectsAddress);
        if (Result != EResult::Success)
        {
            USS_ERROR("Failed to initialize object array");
            return Result;
        }

        USS_LOG("Object array initialized with %d objects", m_pObjectArray->Num());

        m_Status.bObjectArrayInitialized = true;
        return EResult::Success;
    }

    EResult FEngineCore::InitializeNamePool()
    {
        USS_LOG("Initializing name pool...");

        m_GNamesAddress = FindGNamesAddress();
        if (m_GNamesAddress == 0)
        {
            USS_ERROR("Failed to find GNames address");
            return EResult::PatternNotFound;
        }

        USS_LOG("GNames at 0x%llX", m_GNamesAddress);

        m_pNamePool = CreateNamePool();
        if (!m_pNamePool)
        {
            USS_ERROR("Failed to create name pool");
            return EResult::Failed;
        }

        EResult Result = m_pNamePool->Initialize(m_GNamesAddress);
        if (Result != EResult::Success)
        {
            USS_ERROR("Failed to initialize name pool");
            return Result;
        }

        USS_LOG("Name pool initialized");

        m_Status.bNamePoolInitialized = true;
        return EResult::Success;
    }

    EResult FEngineCore::InitializeHooks()
    {
        USS_LOG("Initializing hooks (STUB - @timmie must implement with Memcury/MinHook)...");

        EResult Result = Hook::Initialize();
        if (Result != EResult::Success)
        {
            USS_WARN("Hook::Initialize() returned %s", ResultToString(Result));
        }

        // TODO: @timmie creates ProcessEvent hook here
        // See HookTypes.h for implementation guide

        m_Status.bHooksInitialized = true;
        return EResult::Success;
    }

    uintptr FEngineCore::FindGObjectsAddress()
    {
        const auto& Version = GetVersionResolver().GetVersionInfo();

        // Pattern varies by version, @timmie might need to check
        FPatternResult Result = { false, 0 };

        if (Version.EngineVersionMajor == 4 && Version.EngineVersionMinor <= 22)
        {
            // UE 4.16-4.22 GObjects pattern
            Result = Memory::FindPatternIDA(
                "48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8B D6"
            );

            if (Result)
            {
                // Resolve relative address
                return Memory::ResolveRelative(Result.Address, 7, 3);
            }
        }
        else
        {
            // UE 4.23+ GObjects pattern
            Result = Memory::FindPatternIDA(
                "48 8B 05 ?? ?? ?? ?? 48 8B 0C C8 48 8D 04 D1"
            );

            if (Result)
            {
                return Memory::ResolveRelative(Result.Address, 7, 3);
            }
        }

        USS_WARN("GObjects pattern not found, using fallback...");

        // Fallback: Return 0 (stub - would implement alternative detection)
        return 0;
    }

    uintptr FEngineCore::FindGNamesAddress()
    {
        const auto& Version = GetVersionResolver().GetVersionInfo();

        FPatternResult Result = { false, 0 };

        if (Version.bUseFNamePool)
        {
            // UE 4.23+ FNamePool pattern
            Result = Memory::FindPatternIDA(
                "48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? C6 05 ?? ?? ?? ?? 01"
            );

            if (Result)
            {
                return Memory::ResolveRelative(Result.Address, 7, 3);
            }
        }
        else
        {
            // Pre-4.23 GNames pattern
            Result = Memory::FindPatternIDA(
                "48 8B 05 ?? ?? ?? ?? 48 85 C0 75 50 B9"
            );

            if (Result)
            {
                return Memory::ResolveRelative(Result.Address, 7, 3);
            }
        }

        USS_WARN("GNames pattern not found, using fallback...");
        return 0;
    }

    uintptr FEngineCore::FindGWorldAddress()
    {
        FPatternResult Result = Memory::FindPatternIDA(
            "48 8B 1D ?? ?? ?? ?? ?? ?? ?? 10 4C 8D 4D ?? 4C"
        );

        if (Result)
        {
            return Memory::ResolveRelative(Result.Address, 7, 3);
        }

        USS_WARN("GWorld pattern not found");
        return 0;
    }

    const FVersionInfo& FEngineCore::GetVersionInfo() const
    {
        return GetVersionResolver().GetVersionInfo();
    }

    const FOffsetTable& FEngineCore::GetOffsets() const
    {
        return GetOffsetResolver().GetOffsets();
    }

    UObjectWrapper FEngineCore::FindObject(const char* FullName) const
    {
        if (!m_pObjectArray || !FullName)
            return UObjectWrapper();

        // Simple linear search - in production would use hash
        int32 Num = m_pObjectArray->Num();
        for (int32 i = 0; i < Num; ++i)
        {
            void* Obj = m_pObjectArray->GetByIndex(i);
            if (Obj)
            {
                UObjectWrapper Wrapper(Obj);
                if (Wrapper.GetFullName() == FullName)
                    return Wrapper;
            }
        }

        return UObjectWrapper();
    }

    UObjectWrapper FEngineCore::FindObjectByName(const char* Name) const
    {
        if (!m_pObjectArray || !Name)
            return UObjectWrapper();

        int32 Num = m_pObjectArray->Num();
        for (int32 i = 0; i < Num; ++i)
        {
            void* Obj = m_pObjectArray->GetByIndex(i);
            if (Obj)
            {
                UObjectWrapper Wrapper(Obj);
                if (Wrapper.GetName() == Name)
                    return Wrapper;
            }
        }

        return UObjectWrapper();
    }

    UClassWrapper FEngineCore::FindClass(const char* ClassName) const
    {
        if (!m_pObjectArray || !ClassName)
            return UClassWrapper();

        // Search for Class with matching name
        std::string TargetName = ClassName;
        int32 Num = m_pObjectArray->Num();

        for (int32 i = 0; i < Num; ++i)
        {
            void* Obj = m_pObjectArray->GetByIndex(i);
            if (Obj)
            {
                UObjectWrapper Wrapper(Obj);

                // Check if this is a UClass
                if (Wrapper.IsA("Class"))
                {
                    if (Wrapper.GetName() == TargetName)
                        return UClassWrapper(Obj);
                }
            }
        }

        return UClassWrapper();
    }

    void* FEngineCore::FindLocalPlayerController() const
    {
        // Search for the local player controller
        // Typically named "PlayerController" or "FortPlayerController"
        if (!m_pObjectArray)
            return nullptr;

        int32 Num = m_pObjectArray->Num();
        for (int32 i = 0; i < Num; ++i)
        {
            void* Obj = m_pObjectArray->GetByIndex(i);
            if (Obj)
            {
                UObjectWrapper Wrapper(Obj);
                std::string ClassName = Wrapper.GetObjectClassName();

                // Look for FortPlayerController or FortPlayerControllerAthena
                if (ClassName.find("FortPlayerController") != std::string::npos &&
                    ClassName.find("_C") != std::string::npos)
                {
                    return Obj;
                }
            }
        }

        return nullptr;
    }

    void* FEngineCore::GetWorld() const
    {
        if (m_GWorldAddress == 0)
            return nullptr;

        void* World = nullptr;
        Memory::Read<void*>(m_GWorldAddress, World);
        return World;
    }

    std::string FEngineCore::GetObjectName(void* Object) const
    {
        if (!Object)
            return "";

        UObjectWrapper Wrapper(Object);
        return Wrapper.GetName();
    }

    std::string FEngineCore::GetObjectClassName(void* Object) const
    {
        if (!Object)
            return "";

        UObjectWrapper Wrapper(Object);
        return Wrapper.GetObjectClassName();
    }

    void* FEngineCore::GetObjectClass(void* Object) const
    {
        if (!Object)
            return nullptr;

        // Read UClass* from UObject (typically at offset 0x10)
        const auto& Offsets = GetOffsets();
        int32 ClassOffset = Offsets.UObject.Class;
        if (ClassOffset == 0)
            ClassOffset = 0x10;  // Default offset

        void* Class = nullptr;
        Memory::Read<void*>(reinterpret_cast<uintptr>(Object) + ClassOffset, Class);
        return Class;
    }

    std::string FEngineCore::GetNameFromIndex(int32 NameIndex) const
    {
        if (!m_pNamePool)
            return "";

        return m_pNamePool->GetNameString(NameIndex);
    }

    IOffsetResolver& FEngineCore::GetOffsetResolver() const
    {
        return FStubOffsetResolver::Get();
    }

}