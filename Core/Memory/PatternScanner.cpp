#include "PatternScanner.h"

PatternScanner* PatternScanner::Get()
{
    static PatternScanner* Instance = new PatternScanner();
    return Instance;
}

uintptr_t PatternScanner::FindGetEngineVersion()
{
    auto Results = Memcury::Scanner::FindPattern("40 53 48 83 EC ?? 48 8B D9 E8 ?? ?? ?? ?? 48 8B C8 41 B8"); // Chapter 1 - Chapter 2
    if (!Results.IsValid())
        return 0;

    return Results.Get();
}

uintptr_t PatternScanner::FindProcessEvent()
{
    auto Results = Memcury::Scanner::FindStringRef(L"BadProperty");
    if (!Results.IsValid())
        return 0;

    uintptr_t Address = Results.Get();
    uint8_t* Ptr = reinterpret_cast<uint8_t*>(Address);

    for (int i = 0; i < 0x500; i++)
    {
        Ptr--;

        if (Ptr[0] == 0xFF && Ptr[1] == 0x90)
        {
            int32_t Offset = *reinterpret_cast<int32_t*>(Ptr + 2);
            return static_cast<uintptr_t>(Offset / 8);
        }
    }

    return 0;
}
