/**
 * UniversalSlashingSimulator - Memory Utilities
 *
 * Provides memory manipulation, pattern scanning, and module
 * information utilities. All pattern scanning is abstracted
 * for future external offset finder integration.
 */

#pragma once

#include "../Common.h"
#include <Psapi.h>

namespace USS
{
    struct FModuleInfo
    {
        uintptr BaseAddress;
        size_t Size;
        const char* Name;
    };

    struct FPatternResult
    {
        bool bFound;
        uintptr Address;

        explicit operator bool() const { return bFound; }
    };

    class Memory
    {
    public:
        USS_NON_COPYABLE(Memory)
        USS_NON_MOVABLE(Memory)

        static EResult Initialize();

        static const FModuleInfo& GetBaseModule();

        // Pattern scanning with mask
        // Pattern: raw bytes to match
        // Mask: 'x' = must match, '?' = wildcard
        static FPatternResult FindPattern(const char* Pattern, const char* Mask);

        static FPatternResult FindPattern(uintptr Start, size_t Size, const char* Pattern, const char* Mask);

        // Pattern scanning with IDA-style signature
        // Example: "48 8B 05 ?? ?? ?? ?? 48 85 C0"
        static FPatternResult FindPatternIDA(const char* Signature);

        // Resolve relative address (for RIP-relative instructions)
        // Address: Address containing the relative offset
        // InstructionSize: Total size of the instruction
        // OffsetPosition: Position of the offset within the instruction
        static uintptr ResolveRelative(uintptr Address, int32 InstructionSize, int32 OffsetPosition);

        template<typename T>
        static bool Read(uintptr Address, T& OutValue);

        template<typename T>
        static bool Write(uintptr Address, const T& Value);

        static bool IsValidAddress(uintptr Address);

    private:
        Memory() = default;
        ~Memory() = default;

        static bool MaskCompare(const uint8* Data, const char* Pattern, const char* Mask);

        static FModuleInfo s_BaseModule;
        static bool s_bInitialized;
    };

    template<typename T>
    bool Memory::Read(uintptr Address, T& OutValue)
    {
        if (!IsValidAddress(Address))
            return false;

        __try
        {
            OutValue = *reinterpret_cast<T*>(Address);
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    template<typename T>
    bool Memory::Write(uintptr Address, const T& Value)
    {
        DWORD OldProtect;
        if (!VirtualProtect(reinterpret_cast<void*>(Address), sizeof(T), PAGE_EXECUTE_READWRITE, &OldProtect))
            return false;

        __try
        {
            *reinterpret_cast<T*>(Address) = Value;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            VirtualProtect(reinterpret_cast<void*>(Address), sizeof(T), OldProtect, &OldProtect);
            return false;
        }

        VirtualProtect(reinterpret_cast<void*>(Address), sizeof(T), OldProtect, &OldProtect);
        return true;
    }

}