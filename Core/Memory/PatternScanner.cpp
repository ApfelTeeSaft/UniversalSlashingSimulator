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
