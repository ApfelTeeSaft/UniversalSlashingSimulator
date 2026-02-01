/**
 * UniversalSlashingSimulator - Crash Handler
 *
 * Catches all unhandled exceptions (C++ and SEH) and logs callstack
 * information before terminating. Essential for debugging DLL injection issues.
 */

#pragma once

#include "../Common.h"

namespace USS
{
    /**
     * Exception types that can be caught
     */
    enum class EExceptionType : uint8
    {
        Unknown = 0,
        CPlusPlusException,     // std::exception and derivatives
        AccessViolation,        // SEH - read/write to invalid memory
        StackOverflow,          // SEH - stack exhaustion
        DivideByZero,           // SEH - integer divide by zero
        IllegalInstruction,     // SEH - invalid CPU instruction
        PrivilegedInstruction,  // SEH - ring 0 instruction in ring 3
        InvalidHandle,          // SEH - invalid handle operation
        HeapCorruption,         // SEH - heap memory corruption
        PureVirtualCall,        // C++ - pure virtual function called
        InvalidParameter,       // CRT - invalid parameter to CRT function
        Other                   // Other SEH exception
    };

    /**
     * Convert exception type to string
     */
    const char* ExceptionTypeToString(EExceptionType Type);

    /**
     * Crash Handler - Global exception handling system
     */
    class FCrashHandler
    {
    public:
        /**
         * Initialize the crash handler
         * Sets up SEH filter, terminate handler, and other exception hooks
         * @return Success if initialized
         */
        static EResult Initialize();

        /**
         * Shutdown the crash handler
         * Restores original exception handlers
         */
        static void Shutdown();

        /**
         * Check if crash handler is initialized
         */
        static bool IsInitialized() { return s_bInitialized; }

        /**
         * Manually trigger a crash dump with current callstack
         * Useful for debugging - call this when you detect an error condition
         */
        static void DumpCallstack(const char* Reason = nullptr);

        /**
         * Set whether to show a message box on crash
         * Default is true
         */
        static void SetShowMessageBox(bool bShow) { s_bShowMessageBox = bShow; }

        /**
         * Set whether to write a minidump file on crash
         * Default is true
         */
        static void SetWriteMinidump(bool bWrite) { s_bWriteMinidump = bWrite; }

    private:
        // Internal handlers
        static LONG WINAPI UnhandledExceptionHandler(EXCEPTION_POINTERS* pExceptionInfo);
        static void TerminateHandler();
        static void PureCallHandler();
        static void InvalidParameterHandler(
            const wchar_t* Expression,
            const wchar_t* Function,
            const wchar_t* File,
            unsigned int Line,
            uintptr_t Reserved);

        // Callstack capture and logging
        static void CaptureAndLogCallstack(CONTEXT* pContext = nullptr);
        static void LogExceptionInfo(EXCEPTION_POINTERS* pExceptionInfo);
        static EExceptionType GetExceptionType(DWORD ExceptionCode);

        // Minidump writing
        static void WriteMinidumpFile(EXCEPTION_POINTERS* pExceptionInfo);

        // State
        static bool s_bInitialized;
        static bool s_bShowMessageBox;
        static bool s_bWriteMinidump;
        static LPTOP_LEVEL_EXCEPTION_FILTER s_pPreviousFilter;
        static std::terminate_handler s_pPreviousTerminate;
    };

}