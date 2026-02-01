/**
 * UniversalSlashingSimulator - Logging System
 *
 * Provides thread-safe logging with multiple output targets.
 * Supports console, file, and debug output logging.
 */

#pragma once

#include "../Common.h"
#include <fstream>
#include <cstdarg>

namespace USS
{
    enum class ELogLevel : uint8
    {
        Trace = 0,
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

    class Log
    {
    public:
        USS_NON_COPYABLE(Log)
        USS_NON_MOVABLE(Log)

        static EResult Initialize(bool bEnableConsole = true, const char* LogFilePath = nullptr);

        static void Shutdown();

        static void Write(ELogLevel Level, const char* Format, ...);

        static void WriteV(ELogLevel Level, const char* Format, va_list Args);

        static void SetMinLevel(ELogLevel Level);

        static const char* GetLevelName(ELogLevel Level);

    private:
        Log() = default;
        ~Log() = default;

        static void WriteInternal(ELogLevel Level, const char* Message);

        static FCriticalSection s_CriticalSection;
        static std::ofstream s_FileStream;
        static ELogLevel s_MinLevel;
        static bool s_bConsoleEnabled;
        static bool s_bFileEnabled;
        static bool s_bInitialized;
    };

}