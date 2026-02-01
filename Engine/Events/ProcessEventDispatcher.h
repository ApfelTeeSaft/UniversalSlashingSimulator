/**
 * UniversalSlashingSimulator - ProcessEvent Dispatcher
 *
 * Provides version-agnostic ProcessEvent handling with automatic
 * parameter parsing based on UFunction reflection.
 *
 * ProcessEvent signature varies slightly between versions:
 * - All versions: void ProcessEvent(UFunction*, void* Parms)
 *
 * Parameter handling varies:
 * - Pre-4.25: Parameters parsed via UProperty chain
 * - 4.25+: Parameters parsed via FProperty chain
 *
 * This dispatcher abstracts the version differences and provides
 * type-safe parameter access for hooked functions.
 */

#pragma once

#include "../../Core/Common.h"
#include "../../Core/Hooks/HookTypes.h"
#include "../Reflection/PropertyIterator.h"
#include <unordered_map>
#include <functional>
#include <string>
#include <any>

namespace USS
{
    /**
     * Parsed function parameter
     */
    struct FParsedParameter
    {
        std::string Name;
        EPropertyType Type;
        int32 Offset;
        int32 Size;
        uint64 Flags;

        // Value storage (for common types)
        union
        {
            bool BoolValue;
            int32 IntValue;
            int64 Int64Value;
            float FloatValue;
            double DoubleValue;
            void* PointerValue;
        };

        // For string/name types
        std::string StringValue;

        FParsedParameter()
            : Type(EPropertyType::Unknown)
            , Offset(0)
            , Size(0)
            , Flags(0)
            , PointerValue(nullptr)
        {}

        bool IsOutParam() const
        {
            return (Flags & EPropertyFlags::CPF_OutParm) != 0;
        }

        bool IsReturnParam() const
        {
            return (Flags & EPropertyFlags::CPF_ReturnParm) != 0;
        }

        bool IsReferenceParam() const
        {
            return (Flags & EPropertyFlags::CPF_ReferenceParm) != 0;
        }
    };

    /**
     * Parsed ProcessEvent call context
     */
    struct FProcessEventContext
    {
        void* Object;               // UObject* calling ProcessEvent
        void* Function;             // UFunction* being called
        void* Parameters;           // Raw parameter block

        std::string ObjectName;
        std::string ObjectClassName;
        std::string FunctionName;

        std::vector<FParsedParameter> Params;

        // Timing info
        double Timestamp;

        // Flags
        bool bIsRPC;                // Remote procedure call
        bool bIsMulticast;          // Multicast delegate
        bool bHasReturnValue;       // Function has return value

        FProcessEventContext()
            : Object(nullptr)
            , Function(nullptr)
            , Parameters(nullptr)
            , Timestamp(0.0)
            , bIsRPC(false)
            , bIsMulticast(false)
            , bHasReturnValue(false)
        {}

        /**
         * Get parameter by name
         */
        const FParsedParameter* GetParam(const char* Name) const;

        /**
         * Get parameter value by name (templated for type safety)
         */
        template<typename T>
        bool GetParamValue(const char* Name, T& OutValue) const;

        /**
         * Set output parameter value
         */
        template<typename T>
        bool SetOutParamValue(const char* Name, const T& Value);
    };

    /**
     * ProcessEvent handler callback type
     */
    using FProcessEventHandler = std::function<bool(FProcessEventContext&)>;

    /**
     * Event filter for selective handling
     */
    struct FEventFilter
    {
        std::string ObjectClassFilter;      // Match object class name (empty = all)
        std::string FunctionNameFilter;     // Match function name (empty = all)
        std::string FunctionNamePrefix;     // Match function name prefix
        bool bServerOnly;                   // Only server RPC calls
        bool bClientOnly;                   // Only client RPC calls

        FEventFilter()
            : bServerOnly(false)
            , bClientOnly(false)
        {}

        bool Matches(const FProcessEventContext& Context) const;
    };

    /**
     * Registered event handler
     */
    struct FRegisteredHandler
    {
        int32 HandlerId;
        std::string Name;
        FEventFilter Filter;
        FProcessEventHandler Handler;
        int32 Priority;             // Higher = called first
        bool bEnabled;

        FRegisteredHandler()
            : HandlerId(0)
            , Priority(0)
            , bEnabled(true)
        {}
    };

    /**
     * ProcessEvent Dispatcher
     *
     * Central hub for ProcessEvent interception. Parses function
     * parameters using version-appropriate reflection and dispatches
     * to registered handlers.
     */
    class FProcessEventDispatcher
    {
    public:
        FProcessEventDispatcher();
        ~FProcessEventDispatcher();

        /**
         * Initialize the dispatcher
         */
        EResult Initialize();

        /**
         * Shutdown and cleanup
         */
        void Shutdown();

        /**
         * Main entry point - called from ProcessEvent hook
         * @param Object - UObject* calling ProcessEvent
         * @param Function - UFunction* being executed
         * @param Parameters - Parameter block
         * @return true to allow original execution, false to skip
         */
        bool OnProcessEvent(void* Object, void* Function, void* Parameters);

        /**
         * Register an event handler
         * @param Name - Handler name for debugging
         * @param Filter - Event filter criteria
         * @param Handler - Callback function
         * @param Priority - Execution priority (higher = first)
         * @return Handler ID for later removal
         */
        int32 RegisterHandler(
            const std::string& Name,
            const FEventFilter& Filter,
            FProcessEventHandler Handler,
            int32 Priority = 0);

        /**
         * Unregister a handler by ID
         */
        void UnregisterHandler(int32 HandlerId);

        /**
         * Enable/disable a handler
         */
        void SetHandlerEnabled(int32 HandlerId, bool bEnabled);

        /**
         * Get handler count
         */
        int32 GetHandlerCount() const { return static_cast<int32>(m_Handlers.size()); }

        /**
         * Check if initialized
         */
        bool IsInitialized() const { return m_bInitialized; }

        // Statistics
        uint64 GetTotalEventsProcessed() const { return m_TotalEventsProcessed; }
        uint64 GetTotalEventsHandled() const { return m_TotalEventsHandled; }
        uint64 GetTotalEventsBlocked() const { return m_TotalEventsBlocked; }

    private:
        /**
         * Parse function parameters into context
         */
        bool ParseParameters(void* Function, void* Parameters, FProcessEventContext& OutContext);

        /**
         * Read parameter value from raw memory
         */
        bool ReadParameterValue(void* Parameters, const FPropertyInfo& PropInfo, FParsedParameter& OutParam);

        /**
         * Write parameter value to raw memory
         */
        bool WriteParameterValue(void* Parameters, const FPropertyInfo& PropInfo, const FParsedParameter& Param);

        /**
         * Get cached function info or parse it
         */
        bool GetFunctionInfo(void* Function, std::vector<FPropertyInfo>& OutParams);

        /**
         * Sort handlers by priority
         */
        void SortHandlers();

        // State
        bool m_bInitialized;
        int32 m_NextHandlerId;

        // Registered handlers (sorted by priority)
        std::vector<FRegisteredHandler> m_Handlers;
        bool m_bHandlersDirty;

        // Function parameter cache (UFunction* -> params)
        std::unordered_map<void*, std::vector<FPropertyInfo>> m_FunctionCache;

        // Statistics
        uint64 m_TotalEventsProcessed;
        uint64 m_TotalEventsHandled;
        uint64 m_TotalEventsBlocked;
    };

    /**
     * Global dispatcher accessor
     */
    FProcessEventDispatcher& GetProcessEventDispatcher();

    //=========================================================================
    // Template Implementations
    //=========================================================================

    template<typename T>
    bool FProcessEventContext::GetParamValue(const char* Name, T& OutValue) const
    {
        const FParsedParameter* Param = GetParam(Name);
        if (!Param)
            return false;

        // Type-specific extraction
        if constexpr (std::is_same_v<T, bool>)
        {
            if (Param->Type == EPropertyType::BoolProperty)
            {
                OutValue = Param->BoolValue;
                return true;
            }
        }
        else if constexpr (std::is_same_v<T, int32>)
        {
            if (Param->Type == EPropertyType::IntProperty ||
                Param->Type == EPropertyType::ByteProperty)
            {
                OutValue = Param->IntValue;
                return true;
            }
        }
        else if constexpr (std::is_same_v<T, int64>)
        {
            if (Param->Type == EPropertyType::Int64Property)
            {
                OutValue = Param->Int64Value;
                return true;
            }
        }
        else if constexpr (std::is_same_v<T, float>)
        {
            if (Param->Type == EPropertyType::FloatProperty)
            {
                OutValue = Param->FloatValue;
                return true;
            }
        }
        else if constexpr (std::is_same_v<T, double>)
        {
            if (Param->Type == EPropertyType::DoubleProperty)
            {
                OutValue = Param->DoubleValue;
                return true;
            }
        }
        else if constexpr (std::is_same_v<T, std::string>)
        {
            if (Param->Type == EPropertyType::StrProperty ||
                Param->Type == EPropertyType::NameProperty)
            {
                OutValue = Param->StringValue;
                return true;
            }
        }
        else if constexpr (std::is_pointer_v<T>)
        {
            if (Param->Type == EPropertyType::ObjectProperty ||
                Param->Type == EPropertyType::ClassProperty)
            {
                OutValue = static_cast<T>(Param->PointerValue);
                return true;
            }
        }

        return false;
    }

    template<typename T>
    bool FProcessEventContext::SetOutParamValue(const char* Name, const T& Value)
    {
        for (auto& Param : Params)
        {
            if (Param.Name == Name && Param.IsOutParam())
            {
                // Write value to parameter memory
                if (Parameters && Param.Offset >= 0)
                {
                    void* ParamPtr = static_cast<uint8*>(Parameters) + Param.Offset;

                    if constexpr (std::is_same_v<T, bool>)
                    {
                        *static_cast<bool*>(ParamPtr) = Value;
                        Param.BoolValue = Value;
                    }
                    else if constexpr (std::is_same_v<T, int32>)
                    {
                        *static_cast<int32*>(ParamPtr) = Value;
                        Param.IntValue = Value;
                    }
                    else if constexpr (std::is_same_v<T, float>)
                    {
                        *static_cast<float*>(ParamPtr) = Value;
                        Param.FloatValue = Value;
                    }
                    // Add more types as needed

                    return true;
                }
            }
        }
        return false;
    }

}