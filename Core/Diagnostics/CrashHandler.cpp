/**
 * UniversalSlashingSimulator - Crash Handler Implementation
 */

#include "CrashHandler.h"
#include "../Logging/Log.h"

#include <DbgHelp.h>
#include <cstdio>
#include <ctime>

#pragma comment(lib, "DbgHelp.lib")

namespace USS
{
    // Static member initialization
    bool FCrashHandler::s_bInitialized = false;
    bool FCrashHandler::s_bShowMessageBox = true;
    bool FCrashHandler::s_bWriteMinidump = true;
    LPTOP_LEVEL_EXCEPTION_FILTER FCrashHandler::s_pPreviousFilter = nullptr;
    std::terminate_handler FCrashHandler::s_pPreviousTerminate = nullptr;

    // Maximum frames to capture in callstack
    constexpr int MAX_CALLSTACK_FRAMES = 64;

    // ========================================================================
    // Exception Type Conversion
    // ========================================================================

    const char* ExceptionTypeToString(EExceptionType Type)
    {
        switch (Type)
        {
        case EExceptionType::CPlusPlusException:     return "C++ Exception";
        case EExceptionType::AccessViolation:        return "Access Violation";
        case EExceptionType::StackOverflow:          return "Stack Overflow";
        case EExceptionType::DivideByZero:           return "Divide By Zero";
        case EExceptionType::IllegalInstruction:     return "Illegal Instruction";
        case EExceptionType::PrivilegedInstruction:  return "Privileged Instruction";
        case EExceptionType::InvalidHandle:          return "Invalid Handle";
        case EExceptionType::HeapCorruption:         return "Heap Corruption";
        case EExceptionType::PureVirtualCall:        return "Pure Virtual Call";
        case EExceptionType::InvalidParameter:       return "Invalid Parameter";
        case EExceptionType::Other:                  return "Other Exception";
        default:                                     return "Unknown Exception";
        }
    }

    EExceptionType FCrashHandler::GetExceptionType(DWORD ExceptionCode)
    {
        switch (ExceptionCode)
        {
        case EXCEPTION_ACCESS_VIOLATION:         return EExceptionType::AccessViolation;
        case EXCEPTION_STACK_OVERFLOW:           return EExceptionType::StackOverflow;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:       return EExceptionType::DivideByZero;
        case EXCEPTION_ILLEGAL_INSTRUCTION:      return EExceptionType::IllegalInstruction;
        case EXCEPTION_PRIV_INSTRUCTION:         return EExceptionType::PrivilegedInstruction;
        case EXCEPTION_INVALID_HANDLE:           return EExceptionType::InvalidHandle;
        case STATUS_HEAP_CORRUPTION:             return EExceptionType::HeapCorruption;
        case 0xE06D7363:                         return EExceptionType::CPlusPlusException; // MSVC C++ exception
        default:                                 return EExceptionType::Other;
        }
    }

    // ========================================================================
    // Initialization / Shutdown
    // ========================================================================

    EResult FCrashHandler::Initialize()
    {
        if (s_bInitialized)
            return EResult::AlreadyInitialized;

        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
        if (!SymInitialize(GetCurrentProcess(), nullptr, TRUE))
        {
            // Non-fatal - callstacks just won't have symbol names
            OutputDebugStringA("[USS] Warning: SymInitialize failed, callstacks will not have symbol names\n");
        }

        s_pPreviousFilter = SetUnhandledExceptionFilter(UnhandledExceptionHandler);

        s_pPreviousTerminate = std::set_terminate(TerminateHandler);

        _set_purecall_handler(PureCallHandler);

        _set_invalid_parameter_handler(InvalidParameterHandler);

        s_bInitialized = true;

        USS_LOG("Crash handler initialized");
        return EResult::Success;
    }

    void FCrashHandler::Shutdown()
    {
        if (!s_bInitialized)
            return;

        if (s_pPreviousFilter)
            SetUnhandledExceptionFilter(s_pPreviousFilter);

        if (s_pPreviousTerminate)
            std::set_terminate(s_pPreviousTerminate);

        SymCleanup(GetCurrentProcess());

        s_bInitialized = false;
    }

    // ========================================================================
    // Callstack Capture
    // ========================================================================

    void FCrashHandler::CaptureAndLogCallstack(CONTEXT* pContext)
    {
        void* Stack[MAX_CALLSTACK_FRAMES];
        WORD FrameCount = 0;

        if (pContext)
        {
            // Use StackWalk64 for more accurate results when we have a context
            STACKFRAME64 StackFrame = {};
            DWORD MachineType;

#ifdef _M_X64
            MachineType = IMAGE_FILE_MACHINE_AMD64;
            StackFrame.AddrPC.Offset = pContext->Rip;
            StackFrame.AddrPC.Mode = AddrModeFlat;
            StackFrame.AddrFrame.Offset = pContext->Rbp;
            StackFrame.AddrFrame.Mode = AddrModeFlat;
            StackFrame.AddrStack.Offset = pContext->Rsp;
            StackFrame.AddrStack.Mode = AddrModeFlat;
#else
            MachineType = IMAGE_FILE_MACHINE_I386;
            StackFrame.AddrPC.Offset = pContext->Eip;
            StackFrame.AddrPC.Mode = AddrModeFlat;
            StackFrame.AddrFrame.Offset = pContext->Ebp;
            StackFrame.AddrFrame.Mode = AddrModeFlat;
            StackFrame.AddrStack.Offset = pContext->Esp;
            StackFrame.AddrStack.Mode = AddrModeFlat;
#endif

            HANDLE hProcess = GetCurrentProcess();
            HANDLE hThread = GetCurrentThread();

            // Make a copy of context since StackWalk64 can modify it
            CONTEXT ContextCopy = *pContext;

            for (WORD i = 0; i < MAX_CALLSTACK_FRAMES; ++i)
            {
                if (!StackWalk64(MachineType, hProcess, hThread, &StackFrame, &ContextCopy,
                    nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
                {
                    break;
                }

                if (StackFrame.AddrPC.Offset == 0)
                    break;

                Stack[FrameCount++] = reinterpret_cast<void*>(StackFrame.AddrPC.Offset);
            }
        }
        else
        {
            // No context provided, capture current callstack
            FrameCount = CaptureStackBackTrace(0, MAX_CALLSTACK_FRAMES, Stack, nullptr);
        }

        // Log callstack header
        Log::Write(ELogLevel::Fatal, "========== CALLSTACK ==========");

        if (FrameCount == 0)
        {
            Log::Write(ELogLevel::Fatal, "  (No frames captured)");
            return;
        }

        // Prepare symbol info buffer
        constexpr size_t MaxNameLen = 256;
        char SymbolBuffer[sizeof(SYMBOL_INFO) + MaxNameLen];
        SYMBOL_INFO* pSymbol = reinterpret_cast<SYMBOL_INFO*>(SymbolBuffer);
        pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
        pSymbol->MaxNameLen = MaxNameLen;

        IMAGEHLP_LINE64 LineInfo = {};
        LineInfo.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

        HANDLE hProcess = GetCurrentProcess();

        for (WORD i = 0; i < FrameCount; ++i)
        {
            DWORD64 Address = reinterpret_cast<DWORD64>(Stack[i]);

            // Get module name
            HMODULE hModule = nullptr;
            char ModuleName[MAX_PATH] = "<unknown>";
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCSTR>(Address), &hModule))
            {
                GetModuleFileNameA(hModule, ModuleName, MAX_PATH);
                char* LastSlash = strrchr(ModuleName, '\\');
                if (LastSlash)
                    memmove(ModuleName, LastSlash + 1, strlen(LastSlash));
            }

            DWORD64 Displacement64 = 0;
            const char* SymbolName = "<unknown>";
            if (SymFromAddr(hProcess, Address, &Displacement64, pSymbol))
            {
                SymbolName = pSymbol->Name;
            }

            DWORD Displacement32 = 0;
            char LocationBuffer[512];
            if (SymGetLineFromAddr64(hProcess, Address, &Displacement32, &LineInfo))
            {
                snprintf(LocationBuffer, sizeof(LocationBuffer),
                    "  [%02d] 0x%llX %s!%s + 0x%llX (%s:%lu)",
                    i, Address, ModuleName, SymbolName, Displacement64,
                    LineInfo.FileName, LineInfo.LineNumber);
            }
            else
            {
                snprintf(LocationBuffer, sizeof(LocationBuffer),
                    "  [%02d] 0x%llX %s!%s + 0x%llX",
                    i, Address, ModuleName, SymbolName, Displacement64);
            }

            Log::Write(ELogLevel::Fatal, "%s", LocationBuffer);
        }

        Log::Write(ELogLevel::Fatal, "================================");
    }

    // ========================================================================
    // Exception Info Logging
    // ========================================================================

    void FCrashHandler::LogExceptionInfo(EXCEPTION_POINTERS* pExceptionInfo)
    {
        if (!pExceptionInfo || !pExceptionInfo->ExceptionRecord)
            return;

        EXCEPTION_RECORD* pRecord = pExceptionInfo->ExceptionRecord;
        EExceptionType Type = GetExceptionType(pRecord->ExceptionCode);

        Log::Write(ELogLevel::Fatal, "");
        Log::Write(ELogLevel::Fatal, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Log::Write(ELogLevel::Fatal, "!!           UNHANDLED EXCEPTION                !!");
        Log::Write(ELogLevel::Fatal, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Log::Write(ELogLevel::Fatal, "");
        Log::Write(ELogLevel::Fatal, "Exception Type: %s", ExceptionTypeToString(Type));
        Log::Write(ELogLevel::Fatal, "Exception Code: 0x%08X", pRecord->ExceptionCode);
        Log::Write(ELogLevel::Fatal, "Exception Address: 0x%llX", reinterpret_cast<uint64>(pRecord->ExceptionAddress));

        if (pRecord->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && pRecord->NumberParameters >= 2)
        {
            const char* AccessType = (pRecord->ExceptionInformation[0] == 0) ? "reading" :
                                     (pRecord->ExceptionInformation[0] == 1) ? "writing" : "executing";
            Log::Write(ELogLevel::Fatal, "Access Type: %s address 0x%llX", AccessType, pRecord->ExceptionInformation[1]);
        }

        if (pExceptionInfo->ContextRecord)
        {
            CONTEXT* pCtx = pExceptionInfo->ContextRecord;
            Log::Write(ELogLevel::Fatal, "");
            Log::Write(ELogLevel::Fatal, "========== REGISTERS ==========");
#ifdef _M_X64
            Log::Write(ELogLevel::Fatal, "RAX: 0x%016llX  RBX: 0x%016llX", pCtx->Rax, pCtx->Rbx);
            Log::Write(ELogLevel::Fatal, "RCX: 0x%016llX  RDX: 0x%016llX", pCtx->Rcx, pCtx->Rdx);
            Log::Write(ELogLevel::Fatal, "RSI: 0x%016llX  RDI: 0x%016llX", pCtx->Rsi, pCtx->Rdi);
            Log::Write(ELogLevel::Fatal, "RBP: 0x%016llX  RSP: 0x%016llX", pCtx->Rbp, pCtx->Rsp);
            Log::Write(ELogLevel::Fatal, "R8:  0x%016llX  R9:  0x%016llX", pCtx->R8, pCtx->R9);
            Log::Write(ELogLevel::Fatal, "R10: 0x%016llX  R11: 0x%016llX", pCtx->R10, pCtx->R11);
            Log::Write(ELogLevel::Fatal, "R12: 0x%016llX  R13: 0x%016llX", pCtx->R12, pCtx->R13);
            Log::Write(ELogLevel::Fatal, "R14: 0x%016llX  R15: 0x%016llX", pCtx->R14, pCtx->R15);
            Log::Write(ELogLevel::Fatal, "RIP: 0x%016llX", pCtx->Rip);
#else
            Log::Write(ELogLevel::Fatal, "EAX: 0x%08X  EBX: 0x%08X", pCtx->Eax, pCtx->Ebx);
            Log::Write(ELogLevel::Fatal, "ECX: 0x%08X  EDX: 0x%08X", pCtx->Ecx, pCtx->Edx);
            Log::Write(ELogLevel::Fatal, "ESI: 0x%08X  EDI: 0x%08X", pCtx->Esi, pCtx->Edi);
            Log::Write(ELogLevel::Fatal, "EBP: 0x%08X  ESP: 0x%08X", pCtx->Ebp, pCtx->Esp);
            Log::Write(ELogLevel::Fatal, "EIP: 0x%08X", pCtx->Eip);
#endif
            Log::Write(ELogLevel::Fatal, "================================");
        }

        Log::Write(ELogLevel::Fatal, "");

        CaptureAndLogCallstack(pExceptionInfo->ContextRecord);
    }

    // ========================================================================
    // Minidump Writing
    // ========================================================================

    void FCrashHandler::WriteMinidumpFile(EXCEPTION_POINTERS* pExceptionInfo)
    {
        if (!s_bWriteMinidump)
            return;

        char DumpPath[MAX_PATH];
        time_t Now = time(nullptr);
        struct tm TimeInfo;
        localtime_s(&TimeInfo, &Now);

        char Timestamp[64];
        strftime(Timestamp, sizeof(Timestamp), "%Y%m%d_%H%M%S", &TimeInfo);
        snprintf(DumpPath, sizeof(DumpPath), "USS_Crash_%s.dmp", Timestamp);

        HANDLE hFile = CreateFileA(DumpPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hFile == INVALID_HANDLE_VALUE)
        {
            Log::Write(ELogLevel::Fatal, "Failed to create minidump file: %s", DumpPath);
            return;
        }

        MINIDUMP_EXCEPTION_INFORMATION ExInfo = {};
        ExInfo.ThreadId = GetCurrentThreadId();
        ExInfo.ExceptionPointers = pExceptionInfo;
        ExInfo.ClientPointers = FALSE;

        BOOL bSuccess = MiniDumpWriteDump(
            GetCurrentProcess(),
            GetCurrentProcessId(),
            hFile,
            static_cast<MINIDUMP_TYPE>(MiniDumpWithDataSegs | MiniDumpWithHandleData | MiniDumpWithThreadInfo),
            pExceptionInfo ? &ExInfo : nullptr,
            nullptr,
            nullptr);

        CloseHandle(hFile);

        if (bSuccess)
        {
            Log::Write(ELogLevel::Fatal, "Minidump written to: %s", DumpPath);
        }
        else
        {
            Log::Write(ELogLevel::Fatal, "Failed to write minidump (error: %lu)", GetLastError());
        }
    }

    // ========================================================================
    // Exception Handlers
    // ========================================================================

    LONG WINAPI FCrashHandler::UnhandledExceptionHandler(EXCEPTION_POINTERS* pExceptionInfo)
    {
        LogExceptionInfo(pExceptionInfo);

        WriteMinidumpFile(pExceptionInfo);

        if (s_bShowMessageBox)
        {
            char Message[512];
            EExceptionType Type = GetExceptionType(pExceptionInfo->ExceptionRecord->ExceptionCode);
            snprintf(Message, sizeof(Message),
                "UniversalSlashingSimulator has crashed!\n\n"
                "Exception: %s (0x%08X)\n\n"
                "A crash dump has been written.\n"
                "Check the log file for detailed callstack information.",
                ExceptionTypeToString(Type),
                pExceptionInfo->ExceptionRecord->ExceptionCode);

            MessageBoxA(nullptr, Message, "USS Crash", MB_OK | MB_ICONERROR);
        }

        if (s_pPreviousFilter)
            return s_pPreviousFilter(pExceptionInfo);

        return EXCEPTION_EXECUTE_HANDLER;
    }

    void FCrashHandler::TerminateHandler()
    {
        Log::Write(ELogLevel::Fatal, "");
        Log::Write(ELogLevel::Fatal, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Log::Write(ELogLevel::Fatal, "!!         std::terminate() CALLED              !!");
        Log::Write(ELogLevel::Fatal, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Log::Write(ELogLevel::Fatal, "");

        std::exception_ptr pException = std::current_exception();
        if (pException)
        {
            try
            {
                std::rethrow_exception(pException);
            }
            catch (const std::exception& e)
            {
                Log::Write(ELogLevel::Fatal, "C++ Exception: %s", e.what());
            }
            catch (...)
            {
                Log::Write(ELogLevel::Fatal, "Unknown C++ exception");
            }
        }

        CaptureAndLogCallstack(nullptr);

        if (s_bShowMessageBox)
        {
            MessageBoxA(nullptr,
                "UniversalSlashingSimulator has terminated unexpectedly!\n\n"
                "An unhandled C++ exception occurred.\n\n"
                "Check the log file for detailed callstack information.",
                "USS Crash", MB_OK | MB_ICONERROR);
        }

        if (s_pPreviousTerminate)
            s_pPreviousTerminate();
        else
            std::abort();
    }

    void FCrashHandler::PureCallHandler()
    {
        Log::Write(ELogLevel::Fatal, "");
        Log::Write(ELogLevel::Fatal, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Log::Write(ELogLevel::Fatal, "!!         PURE VIRTUAL FUNCTION CALL           !!");
        Log::Write(ELogLevel::Fatal, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Log::Write(ELogLevel::Fatal, "");

        CaptureAndLogCallstack(nullptr);

        if (s_bShowMessageBox)
        {
            MessageBoxA(nullptr,
                "UniversalSlashingSimulator has crashed!\n\n"
                "A pure virtual function was called.\n\n"
                "Check the log file for detailed callstack information.",
                "USS Crash", MB_OK | MB_ICONERROR);
        }

        std::abort();
    }

    void FCrashHandler::InvalidParameterHandler(
        const wchar_t* Expression,
        const wchar_t* Function,
        const wchar_t* File,
        unsigned int Line,
        uintptr_t Reserved)
    {
        (void)Reserved;

        Log::Write(ELogLevel::Fatal, "");
        Log::Write(ELogLevel::Fatal, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Log::Write(ELogLevel::Fatal, "!!         INVALID CRT PARAMETER                !!");
        Log::Write(ELogLevel::Fatal, "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
        Log::Write(ELogLevel::Fatal, "");

        if (Expression)
            Log::Write(ELogLevel::Fatal, "Expression: %ls", Expression);
        if (Function)
            Log::Write(ELogLevel::Fatal, "Function: %ls", Function);
        if (File)
            Log::Write(ELogLevel::Fatal, "File: %ls:%u", File, Line);

        CaptureAndLogCallstack(nullptr);

        if (s_bShowMessageBox)
        {
            MessageBoxA(nullptr,
                "UniversalSlashingSimulator has crashed!\n\n"
                "An invalid parameter was passed to a CRT function.\n\n"
                "Check the log file for detailed callstack information.",
                "USS Crash", MB_OK | MB_ICONERROR);
        }

        std::abort();
    }

    // ========================================================================
    // Manual Dump
    // ========================================================================

    void FCrashHandler::DumpCallstack(const char* Reason)
    {
        Log::Write(ELogLevel::Warning, "");
        Log::Write(ELogLevel::Warning, "========== MANUAL CALLSTACK DUMP ==========");
        if (Reason)
            Log::Write(ELogLevel::Warning, "Reason: %s", Reason);
        Log::Write(ELogLevel::Warning, "");

        CaptureAndLogCallstack(nullptr);
    }

}