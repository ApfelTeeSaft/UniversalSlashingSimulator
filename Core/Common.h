/**
 * UniversalSlashingSimulator - Common Types and Definitions
 *
 * This file contains common type definitions, macros, and includes
 * used throughout the project. All platform-specific definitions
 * are centralized here.
 */

#pragma once

#include <cstdint>
#include <cstddef>
#include <string>
#include <memory>
#include <functional>
#include <vector>
#include <unordered_map>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace USS
{
    using int8 = int8_t;
    using int16 = int16_t;
    using int32 = int32_t;
    using int64 = int64_t;
    using uint8 = uint8_t;
    using uint16 = uint16_t;
    using uint32 = uint32_t;
    using uint64 = uint64_t;

    using uintptr = uintptr_t;
    using intptr = intptr_t;

    enum class EResult : uint8
    {
        Success = 0,
        Failed,
        NotSupported,
        InvalidVersion,
        PatternNotFound,
        HookFailed,
        AlreadyInitialized,
        NotInitialized,

        InvalidState,
        InvalidParameter,

        InsufficientResources,
        InventoryFull,
        ItemNotFound,

        BuildingNotFound,
        BuildLimitReached,
        InvalidPlacement,

        TrapNotFound,
        TrapNotReady
    };

    inline const char* ResultToString(EResult Result)
    {
        switch (Result)
        {
        case EResult::Success:              return "Success";
        case EResult::Failed:               return "Failed";
        case EResult::NotSupported:         return "NotSupported";
        case EResult::InvalidVersion:       return "InvalidVersion";
        case EResult::PatternNotFound:      return "PatternNotFound";
        case EResult::HookFailed:           return "HookFailed";
        case EResult::AlreadyInitialized:   return "AlreadyInitialized";
        case EResult::NotInitialized:       return "NotInitialized";
        case EResult::InvalidState:         return "InvalidState";
        case EResult::InvalidParameter:     return "InvalidParameter";
        case EResult::InsufficientResources:return "InsufficientResources";
        case EResult::InventoryFull:        return "InventoryFull";
        case EResult::ItemNotFound:         return "ItemNotFound";
        case EResult::BuildingNotFound:     return "BuildingNotFound";
        case EResult::BuildLimitReached:    return "BuildLimitReached";
        case EResult::InvalidPlacement:     return "InvalidPlacement";
        case EResult::TrapNotFound:         return "TrapNotFound";
        case EResult::TrapNotReady:         return "TrapNotReady";
        default:                            return "Unknown";
        }
    }

    #define USS_INTERFACE class

    #define USS_NON_COPYABLE(ClassName) \
        ClassName(const ClassName&) = delete; \
        ClassName& operator=(const ClassName&) = delete;

    #define USS_NON_MOVABLE(ClassName) \
        ClassName(ClassName&&) = delete; \
        ClassName& operator=(ClassName&&) = delete;

    #ifdef USS_DEBUG
        #define USS_LOG(fmt, ...) ::USS::Log::Write(::USS::ELogLevel::Info, fmt, ##__VA_ARGS__)
        #define USS_WARN(fmt, ...) ::USS::Log::Write(::USS::ELogLevel::Warning, fmt, ##__VA_ARGS__)
        #define USS_ERROR(fmt, ...) ::USS::Log::Write(::USS::ELogLevel::Error, fmt, ##__VA_ARGS__)
    #else
        #define USS_LOG(fmt, ...) ((void)0)
        #define USS_WARN(fmt, ...) ((void)0)
        #define USS_ERROR(fmt, ...) ((void)0)
    #endif

    #define USS_FATAL(fmt, ...) ::USS::Log::Write(::USS::ELogLevel::Fatal, fmt, ##__VA_ARGS__)

}