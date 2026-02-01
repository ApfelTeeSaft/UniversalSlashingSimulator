/**
 * UniversalSlashingSimulator - Logging System Implementation
 */

#include "Log.h"
#include <cstdio>
#include <ctime>

namespace USS
{
    FCriticalSection Log::s_CriticalSection;
    std::ofstream Log::s_FileStream;
    ELogLevel Log::s_MinLevel = ELogLevel::Info;
    bool Log::s_bConsoleEnabled = false;
    bool Log::s_bFileEnabled = false;
    bool Log::s_bInitialized = false;

    EResult Log::Initialize(bool bEnableConsole, const char* LogFilePath)
    {
        FScopedLock Lock(s_CriticalSection);

        if (s_bInitialized)
            return EResult::AlreadyInitialized;

        if (bEnableConsole)
        {
            if (AllocConsole())
            {
                FILE* pFile = nullptr;
                freopen_s(&pFile, "CONOUT$", "w", stdout);
                freopen_s(&pFile, "CONOUT$", "w", stderr);
                s_bConsoleEnabled = true;
            }
        }

        if (LogFilePath != nullptr)
        {
            s_FileStream.open(LogFilePath, std::ios::out | std::ios::trunc);
            if (s_FileStream.is_open())
            {
                s_bFileEnabled = true;
            }
        }

        s_bInitialized = true;
        return EResult::Success;
    }

    void Log::Shutdown()
    {
        FScopedLock Lock(s_CriticalSection);

        if (!s_bInitialized)
            return;

        if (s_bFileEnabled && s_FileStream.is_open())
        {
            s_FileStream.close();
        }

        if (s_bConsoleEnabled)
        {
            FreeConsole();
        }

        s_bConsoleEnabled = false;
        s_bFileEnabled = false;
        s_bInitialized = false;
    }

    void Log::Write(ELogLevel Level, const char* Format, ...)
    {
        va_list Args;
        va_start(Args, Format);
        WriteV(Level, Format, Args);
        va_end(Args);
    }

    void Log::WriteV(ELogLevel Level, const char* Format, va_list Args)
    {
        if (Level < s_MinLevel)
            return;

        char Buffer[4096];
        vsnprintf(Buffer, sizeof(Buffer), Format, Args);
        Buffer[sizeof(Buffer) - 1] = '\0';

        WriteInternal(Level, Buffer);
    }

    void Log::SetMinLevel(ELogLevel Level)
    {
        FScopedLock Lock(s_CriticalSection);
        s_MinLevel = Level;
    }

    const char* Log::GetLevelName(ELogLevel Level)
    {
        switch (Level)
        {
        case ELogLevel::Trace:   return "TRACE";
        case ELogLevel::Debug:   return "DEBUG";
        case ELogLevel::Info:    return "INFO";
        case ELogLevel::Warning: return "WARN";
        case ELogLevel::Error:   return "ERROR";
        case ELogLevel::Fatal:   return "FATAL";
        default:                 return "UNKNOWN";
        }
    }

    void Log::WriteInternal(ELogLevel Level, const char* Message)
    {
        FScopedLock Lock(s_CriticalSection);

        if (!s_bInitialized)
            return;

        time_t Now = time(nullptr);
        struct tm TimeInfo;
        localtime_s(&TimeInfo, &Now);

        char Timestamp[32];
        strftime(Timestamp, sizeof(Timestamp), "%H:%M:%S", &TimeInfo);

        // Format: [HH:MM:SS] [LEVEL] Message
        char FormattedMessage[4200];
        snprintf(FormattedMessage, sizeof(FormattedMessage),
            "[%s] [%s] %s\n", Timestamp, GetLevelName(Level), Message);

        if (s_bConsoleEnabled)
        {
            HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
            WORD Color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;

            switch (Level)
            {
            case ELogLevel::Trace:
            case ELogLevel::Debug:
                Color = FOREGROUND_INTENSITY;
                break;
            case ELogLevel::Info:
                Color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                break;
            case ELogLevel::Warning:
                Color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                break;
            case ELogLevel::Error:
            case ELogLevel::Fatal:
                Color = FOREGROUND_RED | FOREGROUND_INTENSITY;
                break;
            }

            SetConsoleTextAttribute(hConsole, Color);
            printf("%s", FormattedMessage);
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }

        if (s_bFileEnabled && s_FileStream.is_open())
        {
            s_FileStream << FormattedMessage;
            s_FileStream.flush();
        }

        OutputDebugStringA(FormattedMessage);
    }

}