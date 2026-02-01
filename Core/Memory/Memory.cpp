/**
 * UniversalSlashingSimulator - Memory Utilities Implementation
 */

#include "Memory.h"
#include "../Logging/Log.h"
#include <sstream>
#include <vector>

namespace USS
{
    FModuleInfo Memory::s_BaseModule = { 0, 0, nullptr };
    bool Memory::s_bInitialized = false;

    EResult Memory::Initialize()
    {
        if (s_bInitialized)
            return EResult::AlreadyInitialized;

        HMODULE hModule = GetModuleHandle(nullptr);
        if (!hModule)
        {
            USS_ERROR("Failed to get base module handle");
            return EResult::Failed;
        }

        MODULEINFO ModInfo = {};
        if (!GetModuleInformation(GetCurrentProcess(), hModule, &ModInfo, sizeof(ModInfo)))
        {
            USS_ERROR("Failed to get module information");
            return EResult::Failed;
        }

        s_BaseModule.BaseAddress = reinterpret_cast<uintptr>(ModInfo.lpBaseOfDll);
        s_BaseModule.Size = ModInfo.SizeOfImage;
        s_BaseModule.Name = "FortniteClient-Win64-Shipping.exe";

        USS_LOG("Memory initialized - Base: 0x%llX, Size: 0x%llX",
            s_BaseModule.BaseAddress, s_BaseModule.Size);

        s_bInitialized = true;
        return EResult::Success;
    }

    const FModuleInfo& Memory::GetBaseModule()
    {
        return s_BaseModule;
    }

    bool Memory::MaskCompare(const uint8* Data, const char* Pattern, const char* Mask)
    {
        for (; *Mask; ++Mask, ++Data, ++Pattern)
        {
            if (*Mask == 'x' && *Data != static_cast<uint8>(*Pattern))
                return false;
        }
        return true;
    }

    FPatternResult Memory::FindPattern(const char* Pattern, const char* Mask)
    {
        if (!s_bInitialized)
            return { false, 0 };

        return FindPattern(s_BaseModule.BaseAddress, s_BaseModule.Size, Pattern, Mask);
    }

    FPatternResult Memory::FindPattern(uintptr Start, size_t Size, const char* Pattern, const char* Mask)
    {
        FPatternResult Result = { false, 0 };

        if (!Pattern || !Mask)
            return Result;

        size_t MaskLength = strlen(Mask);
        if (Size < MaskLength)
            return Result;

        size_t SearchSize = Size - MaskLength;

        for (size_t i = 0; i < SearchSize; ++i)
        {
            const uint8* Address = reinterpret_cast<const uint8*>(Start + i);

            if (MaskCompare(Address, Pattern, Mask))
            {
                Result.bFound = true;
                Result.Address = Start + i;
                return Result;
            }
        }

        return Result;
    }

    FPatternResult Memory::FindPatternIDA(const char* Signature)
    {
        FPatternResult Result = { false, 0 };

        if (!Signature || !s_bInitialized)
            return Result;

        std::vector<char> Pattern;
        std::vector<char> Mask;

        std::istringstream Stream(Signature);
        std::string Token;

        while (Stream >> Token)
        {
            if (Token == "?" || Token == "??")
            {
                Pattern.push_back(0x00);
                Mask.push_back('?');
            }
            else
            {
                try
                {
                    int Value = std::stoi(Token, nullptr, 16);
                    Pattern.push_back(static_cast<char>(Value));
                    Mask.push_back('x');
                }
                catch (...)
                {
                    USS_ERROR("Invalid signature token: %s", Token.c_str());
                    return Result;
                }
            }
        }

        if (Pattern.empty())
            return Result;

        Pattern.push_back('\0');
        Mask.push_back('\0');

        return FindPattern(Pattern.data(), Mask.data());
    }

    uintptr Memory::ResolveRelative(uintptr Address, int32 InstructionSize, int32 OffsetPosition)
    {
        if (!IsValidAddress(Address))
            return 0;

        int32 Offset = 0;
        if (!Read<int32>(Address + OffsetPosition, Offset))
            return 0;

        return Address + InstructionSize + Offset;
    }

    bool Memory::IsValidAddress(uintptr Address)
    {
        if (Address == 0)
            return false;

        MEMORY_BASIC_INFORMATION MemInfo = {};
        if (VirtualQuery(reinterpret_cast<void*>(Address), &MemInfo, sizeof(MemInfo)) == 0)
            return false;

        if (MemInfo.State != MEM_COMMIT)
            return false;

        if (MemInfo.Protect & (PAGE_NOACCESS | PAGE_GUARD))
            return false;

        return true;
    }

}