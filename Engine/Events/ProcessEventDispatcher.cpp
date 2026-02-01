/**
 * UniversalSlashingSimulator - ProcessEvent Dispatcher Implementation
 */

#include "ProcessEventDispatcher.h"
#include "../../Core/Memory/Memory.h"
#include "../../Core/Logging/Log.h"
#include "../EngineCore.h"
#include <algorithm>

namespace USS
{
    //=========================================================================
    // FProcessEventContext Methods
    //=========================================================================

    const FParsedParameter* FProcessEventContext::GetParam(const char* Name) const
    {
        if (!Name)
            return nullptr;

        for (const auto& Param : Params)
        {
            if (Param.Name == Name)
            {
                return &Param;
            }
        }
        return nullptr;
    }

    //=========================================================================
    // FEventFilter Methods
    //=========================================================================

    bool FEventFilter::Matches(const FProcessEventContext& Context) const
    {
        if (!ObjectClassFilter.empty())
        {
            if (Context.ObjectClassName.find(ObjectClassFilter) == std::string::npos)
                return false;
        }

        if (!FunctionNameFilter.empty())
        {
            if (Context.FunctionName != FunctionNameFilter)
                return false;
        }

        if (!FunctionNamePrefix.empty())
        {
            if (Context.FunctionName.find(FunctionNamePrefix) != 0)
                return false;
        }

        if (bServerOnly && !Context.bIsRPC)
            return false;

        if (bClientOnly && Context.bIsRPC)
            return false;

        return true;
    }

    //=========================================================================
    // FProcessEventDispatcher Implementation
    //=========================================================================

    static FProcessEventDispatcher g_ProcessEventDispatcher;

    FProcessEventDispatcher& GetProcessEventDispatcher()
    {
        return g_ProcessEventDispatcher;
    }

    FProcessEventDispatcher::FProcessEventDispatcher()
        : m_bInitialized(false)
        , m_NextHandlerId(1)
        , m_bHandlersDirty(false)
        , m_TotalEventsProcessed(0)
        , m_TotalEventsHandled(0)
        , m_TotalEventsBlocked(0)
    {
    }

    FProcessEventDispatcher::~FProcessEventDispatcher()
    {
        Shutdown();
    }

    EResult FProcessEventDispatcher::Initialize()
    {
        if (m_bInitialized)
            return EResult::AlreadyInitialized;

        USS_LOG("Initializing ProcessEvent Dispatcher...");

        if (!GetPropertyIterator().IsInitialized())
        {
            EResult Result = GetPropertyIterator().Initialize();
            if (Result != EResult::Success)
            {
                USS_ERROR("Failed to initialize property iterator");
                return Result;
            }
        }

        m_Handlers.clear();
        m_FunctionCache.clear();
        m_TotalEventsProcessed = 0;
        m_TotalEventsHandled = 0;
        m_TotalEventsBlocked = 0;

        USS_LOG("ProcessEvent Dispatcher initialized");

        m_bInitialized = true;
        return EResult::Success;
    }

    void FProcessEventDispatcher::Shutdown()
    {
        if (!m_bInitialized)
            return;

        USS_LOG("Shutting down ProcessEvent Dispatcher...");
        USS_LOG("  Events processed: %llu", m_TotalEventsProcessed);
        USS_LOG("  Events handled: %llu", m_TotalEventsHandled);
        USS_LOG("  Events blocked: %llu", m_TotalEventsBlocked);

        m_Handlers.clear();
        m_FunctionCache.clear();

        m_bInitialized = false;
    }

    bool FProcessEventDispatcher::OnProcessEvent(void* Object, void* Function, void* Parameters)
    {
        if (!m_bInitialized)
            return true;

        ++m_TotalEventsProcessed;

        if (m_bHandlersDirty)
        {
            SortHandlers();
        }

        if (m_Handlers.empty())
            return true;

        FProcessEventContext Context;
        Context.Object = Object;
        Context.Function = Function;
        Context.Parameters = Parameters;

        Context.ObjectName = GetEngineCore().GetObjectName(Object);
        Context.ObjectClassName = GetEngineCore().GetObjectClassName(Object);
        Context.FunctionName = GetEngineCore().GetObjectName(Function);

        Context.bIsRPC = (Context.FunctionName.find("Server") == 0) ||
                         (Context.FunctionName.find("Client") == 0);
        Context.bIsMulticast = (Context.FunctionName.find("Multicast") != std::string::npos);

        bool bNeedsParsing = false;
        for (const auto& Handler : m_Handlers)
        {
            if (Handler.bEnabled && Handler.Filter.Matches(Context))
            {
                bNeedsParsing = true;
                break;
            }
        }

        if (bNeedsParsing)
        {
            ParseParameters(Function, Parameters, Context);
        }

        bool bAllowExecution = true;

        for (const auto& Handler : m_Handlers)
        {
            if (!Handler.bEnabled)
                continue;

            if (!Handler.Filter.Matches(Context))
                continue;

            ++m_TotalEventsHandled;

            bool bContinue = Handler.Handler(Context);

            if (!bContinue)
            {
                bAllowExecution = false;
                ++m_TotalEventsBlocked;
                break;
            }
        }

        return bAllowExecution;
    }

    int32 FProcessEventDispatcher::RegisterHandler(
        const std::string& Name,
        const FEventFilter& Filter,
        FProcessEventHandler Handler,
        int32 Priority)
    {
        if (!Handler)
            return 0;

        FRegisteredHandler Registration;
        Registration.HandlerId = m_NextHandlerId++;
        Registration.Name = Name;
        Registration.Filter = Filter;
        Registration.Handler = std::move(Handler);
        Registration.Priority = Priority;
        Registration.bEnabled = true;

        m_Handlers.push_back(std::move(Registration));
        m_bHandlersDirty = true;

        USS_LOG("Registered ProcessEvent handler: %s (ID: %d, Priority: %d)",
            Name.c_str(), Registration.HandlerId, Priority);

        return Registration.HandlerId;
    }

    void FProcessEventDispatcher::UnregisterHandler(int32 HandlerId)
    {
        auto It = std::remove_if(m_Handlers.begin(), m_Handlers.end(),
            [HandlerId](const FRegisteredHandler& H) {
                return H.HandlerId == HandlerId;
            });

        if (It != m_Handlers.end())
        {
            USS_LOG("Unregistered ProcessEvent handler ID: %d", HandlerId);
            m_Handlers.erase(It, m_Handlers.end());
        }
    }

    void FProcessEventDispatcher::SetHandlerEnabled(int32 HandlerId, bool bEnabled)
    {
        for (auto& Handler : m_Handlers)
        {
            if (Handler.HandlerId == HandlerId)
            {
                Handler.bEnabled = bEnabled;
                USS_LOG("Handler %d %s", HandlerId, bEnabled ? "enabled" : "disabled");
                break;
            }
        }
    }

    bool FProcessEventDispatcher::ParseParameters(void* Function, void* Parameters, FProcessEventContext& OutContext)
    {
        if (!Function || !Parameters)
            return false;

        std::vector<FPropertyInfo> ParamInfos;
        if (!GetFunctionInfo(Function, ParamInfos))
            return false;

        OutContext.Params.clear();
        OutContext.Params.reserve(ParamInfos.size());

        for (const auto& PropInfo : ParamInfos)
        {
            if (!(PropInfo.PropertyFlags & EPropertyFlags::CPF_Parm))
                continue;

            FParsedParameter Param;
            if (ReadParameterValue(Parameters, PropInfo, Param))
            {
                if (PropInfo.PropertyFlags & EPropertyFlags::CPF_ReturnParm)
                {
                    OutContext.bHasReturnValue = true;
                }
                OutContext.Params.push_back(std::move(Param));
            }
        }

        return true;
    }

    bool FProcessEventDispatcher::ReadParameterValue(void* Parameters, const FPropertyInfo& PropInfo, FParsedParameter& OutParam)
    {
        if (!Parameters)
            return false;

        OutParam.Name = PropInfo.Name;
        OutParam.Type = PropInfo.Type;
        OutParam.Offset = PropInfo.Offset;
        OutParam.Size = PropInfo.ElementSize;
        OutParam.Flags = PropInfo.PropertyFlags;

        uintptr ParamAddr = reinterpret_cast<uintptr>(Parameters) + PropInfo.Offset;

        switch (PropInfo.Type)
        {
        case EPropertyType::BoolProperty:
            Memory::Read<bool>(ParamAddr, OutParam.BoolValue);
            break;

        case EPropertyType::ByteProperty:
        case EPropertyType::Int8Property:
        {
            int8 Value = 0;
            Memory::Read<int8>(ParamAddr, Value);
            OutParam.IntValue = Value;
            break;
        }

        case EPropertyType::Int16Property:
        case EPropertyType::UInt16Property:
        {
            int16 Value = 0;
            Memory::Read<int16>(ParamAddr, Value);
            OutParam.IntValue = Value;
            break;
        }

        case EPropertyType::IntProperty:
        case EPropertyType::UInt32Property:
            Memory::Read<int32>(ParamAddr, OutParam.IntValue);
            break;

        case EPropertyType::Int64Property:
        case EPropertyType::UInt64Property:
            Memory::Read<int64>(ParamAddr, OutParam.Int64Value);
            break;

        case EPropertyType::FloatProperty:
            Memory::Read<float>(ParamAddr, OutParam.FloatValue);
            break;

        case EPropertyType::DoubleProperty:
            Memory::Read<double>(ParamAddr, OutParam.DoubleValue);
            break;

        case EPropertyType::ObjectProperty:
        case EPropertyType::ClassProperty:
        case EPropertyType::InterfaceProperty:
        case EPropertyType::WeakObjectProperty:
        case EPropertyType::LazyObjectProperty:
        case EPropertyType::SoftObjectProperty:
            Memory::Read<void*>(ParamAddr, OutParam.PointerValue);
            break;

        case EPropertyType::NameProperty:
        {
            // FName is typically int32 ComparisonIndex + int32 Number
            int32 NameIndex = 0;
            Memory::Read<int32>(ParamAddr, NameIndex);
            OutParam.StringValue = GetEngineCore().GetNameFromIndex(NameIndex);
            break;
        }

        case EPropertyType::StrProperty:
        {
            // FString is TArray<TCHAR>
            // Try to read as FString
            uintptr DataPtr = 0;
            int32 Len = 0;
            Memory::Read<uintptr>(ParamAddr, DataPtr);
            Memory::Read<int32>(ParamAddr + sizeof(uintptr), Len);

            if (DataPtr && Len > 0 && Len < 4096)
            {
                // Read as wide string
                std::wstring WideStr(Len, L'\0');
                for (int32 i = 0; i < Len; ++i)
                {
                    wchar_t Ch = 0;
                    Memory::Read<wchar_t>(DataPtr + (i * sizeof(wchar_t)), Ch);
                    WideStr[i] = Ch;
                }

                // Convert to narrow
                OutParam.StringValue.reserve(Len);
                for (wchar_t Ch : WideStr)
                {
                    if (Ch == 0) break;
                    OutParam.StringValue += static_cast<char>(Ch);
                }
            }
            break;
        }

        case EPropertyType::StructProperty:
            // Store pointer to struct data
            OutParam.PointerValue = reinterpret_cast<void*>(ParamAddr);
            break;

        case EPropertyType::ArrayProperty:
            // Store pointer to array
            OutParam.PointerValue = reinterpret_cast<void*>(ParamAddr);
            break;

        default:
            // Store raw pointer for unknown types
            OutParam.PointerValue = reinterpret_cast<void*>(ParamAddr);
            break;
        }

        return true;
    }

    bool FProcessEventDispatcher::WriteParameterValue(void* Parameters, const FPropertyInfo& PropInfo, const FParsedParameter& Param)
    {
        if (!Parameters)
            return false;

        uintptr ParamAddr = reinterpret_cast<uintptr>(Parameters) + PropInfo.Offset;

        switch (PropInfo.Type)
        {
        case EPropertyType::BoolProperty:
            return Memory::Write<bool>(ParamAddr, Param.BoolValue);

        case EPropertyType::IntProperty:
            return Memory::Write<int32>(ParamAddr, Param.IntValue);

        case EPropertyType::Int64Property:
            return Memory::Write<int64>(ParamAddr, Param.Int64Value);

        case EPropertyType::FloatProperty:
            return Memory::Write<float>(ParamAddr, Param.FloatValue);

        case EPropertyType::DoubleProperty:
            return Memory::Write<double>(ParamAddr, Param.DoubleValue);

        case EPropertyType::ObjectProperty:
            return Memory::Write<void*>(ParamAddr, Param.PointerValue);

        default:
            return false;
        }
    }

    bool FProcessEventDispatcher::GetFunctionInfo(void* Function, std::vector<FPropertyInfo>& OutParams)
    {
        if (!Function)
            return false;

        // Check cache
        auto It = m_FunctionCache.find(Function);
        if (It != m_FunctionCache.end())
        {
            OutParams = It->second;
            return true;
        }

        // Parse function parameters using property iterator
        std::vector<FPropertyInfo> Params;

        GetPropertyIterator().ForEachProperty(Function,
            [&Params](const FPropertyInfo& Info) -> bool {
                if (Info.PropertyFlags & EPropertyFlags::CPF_Parm)
                {
                    Params.push_back(Info);
                }
                return true;  // Continue
            },
            false);  // Don't include super (functions don't inherit params)

        // Sort by offset to ensure correct parameter order
        std::sort(Params.begin(), Params.end(),
            [](const FPropertyInfo& A, const FPropertyInfo& B) {
                return A.Offset < B.Offset;
            });

        // Cache result
        m_FunctionCache[Function] = Params;
        OutParams = std::move(Params);

        return true;
    }

    void FProcessEventDispatcher::SortHandlers()
    {
        std::sort(m_Handlers.begin(), m_Handlers.end(),
            [](const FRegisteredHandler& A, const FRegisteredHandler& B) {
                return A.Priority > B.Priority;
            });

        m_bHandlersDirty = false;
    }

}