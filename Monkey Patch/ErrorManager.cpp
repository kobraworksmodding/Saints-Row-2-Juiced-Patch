#include <cstdio>
#include <cstring>
#include <stdio.h>
#include <windows.h>
#include <dbghelp.h>
#pragma comment (lib, "dbghelp.lib")
#include "GameConfig.h"
#include "FileLogger.h"
#include "ErrorManager.h"

//~ #define NUM_ERRORCODES 19
#define NUM_ERRORCODES 19
#define ERR_LENGTH 4096

// Code reused from reTHAWed.
// Special thanks to Zedek
namespace ErrorManager
{
    bool b_Enabled = false;
    bool b_HandlerAssigned = false;
    bool b_HasErrored = false;
    bool b_IgnoreVectoredExceptions = false;
    int  gameHasBrokeCounter = 0;

    PVOID curVectorHandler;

    ErrorEntry error_codes[NUM_ERRORCODES] = {
        {0xC0000005, "EXCEPTION_ACCESS_VIOLATION", EHR_TERMINATE},
        {0xC000008C, "EXCEPTION_ARRAY_BOUNDS_EXCEEDED", EHR_TERMINATE},
        {0x80000003, "EXCEPTION_BREAKPOINT", EHR_CONTINUE},
        {0x80000002, "EXCEPTION_DATATYPE_MISALIGNMENT", EHR_TERMINATE},
        {0xC000008D, "EXCEPTION_FLT_DENORMAL_OPERAND", EHR_TERMINATE},
        {0xC000008F, "EXCEPTION_FLT_INEXACT_RESULT", EHR_TERMINATE},
        {0xC0000090, "EXCEPTION_FLT_INVALID_OPERATION", EHR_TERMINATE},
        {0xC0000091, "EXCEPTION_FLT_OVERFLOW", EHR_TERMINATE},
        {0xC0000092, "EXCEPTION_FLT_STACK_CHECK", EHR_TERMINATE},
        {0xC0000093, "EXCEPTION_FLT_UNDERFLOW", EHR_TERMINATE},
        {0xC000001D, "EXCEPTION_ILLEGAL_INSTRUCTION", EHR_TERMINATE},
        {0xC0000006, "EXCEPTION_IN_PAGE_ERROR", EHR_TERMINATE},
        {0xC0000094, "EXCEPTION_INT_DIVIDE_BY_ZERO", EHR_TERMINATE},
        {0xC0000095, "EXCEPTION_INT_OVERFLOW", EHR_TERMINATE},
        {0xC0000026, "EXCEPTION_INVALID_DISPOSITION", EHR_CONTINUE},
        {0xC0000025, "EXCEPTION_NONCONTINUABLE_EXCEPTION", EHR_TERMINATE},
        {0xC0000096, "EXCEPTION_PRIV_INSTRUCTION", EHR_TERMINATE},
        {0x80000004, "EXCEPTION_SINGLE_STEP", EHR_CONTINUE},
        {0xC00000FD, "EXCEPTION_STACK_OVERFLOW", EHR_TERMINATE}
    };

    // ------------------------------------

    void FormatExceptionRecords(PCONTEXT context, char* buffer)
    {
        context->ContextFlags = CONTEXT_FULL;

        STACKFRAME64 StackFrame;
        memset(&StackFrame, 0, sizeof(StackFrame));

        // process, thread, i386, eip, ebp, esp
        // process, thread, machine, pc, frameptr, stackptr

        StackFrame.AddrPC.Offset = context->Eip;
        StackFrame.AddrFrame.Offset = context->Ebp;
        StackFrame.AddrStack.Offset = context->Esp;

        StackFrame.AddrPC.Mode = AddrModeFlat;
        StackFrame.AddrFrame.Mode = AddrModeFlat;
        StackFrame.AddrStack.Mode = AddrModeFlat;

        HANDLE curThread = GetCurrentThread();
        HANDLE curProcess = GetCurrentProcess();

        snprintf(buffer, ERR_LENGTH, "%s\n\n", buffer);

        // Let's print our registry values (as QBKeys!)
        // Some of them might be stored in memory and could be helpful

        snprintf(buffer, ERR_LENGTH, "%sEAX: 0x%08x\n", buffer, context->Eax);
        snprintf(buffer, ERR_LENGTH, "%sEBX: 0x%08x\n", buffer, context->Ebx);
        snprintf(buffer, ERR_LENGTH, "%sECX: 0x%08x\n", buffer, context->Ecx);
        snprintf(buffer, ERR_LENGTH, "%sEDX: 0x%08x\n", buffer, context->Edx);
        snprintf(buffer, ERR_LENGTH, "%sEBP: 0x%08x\n", buffer, context->Ebp);
        snprintf(buffer, ERR_LENGTH, "%sEDI: 0x%08x\n", buffer, context->Edi);
        snprintf(buffer, ERR_LENGTH, "%sESP: 0x%08x\n", buffer, context->Esp);

        while (true)
        {
            if (StackWalk64(IMAGE_FILE_MACHINE_I386, curProcess, curThread, &StackFrame, context, NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL) == 0)
                break;

            if (StackFrame.AddrFrame.Offset == 0)
                break;

            snprintf(buffer, ERR_LENGTH, "%s\n[0x%08p]", buffer, StackFrame.AddrPC.Offset);
        }
    }

    // ------------------------------------
    // Core exception handler!
    // Returns true to continue execution, false to quit

    ExceptionHandlerResult HandleException_Core(_EXCEPTION_POINTERS* ExceptionInfo)
    {
        // No longer interested in handling errors.
        SetUnhandledExceptionFilter(0);

        if (!ExceptionInfo)
        {
            Logger::TypedLog(CHN_DLL, "ExceptionInfo missing.\n");
            return EHR_CONTINUE;
        }
        uint32_t crashAddr = ExceptionInfo->ContextRecord->Eip;
        PEXCEPTION_RECORD record = ExceptionInfo->ExceptionRecord;

        uint32_t exceptionCode = record->ExceptionCode;

        // Special exception for breakpoint
        // (We would like to TERMINATE the application!)
        //
        // (This seems to only be encountered when the program exits)

        // ------------------

        ExceptionHandlerResult exceptionResult = EHR_CONTINUE;
        const char* exceptionString = "UNKNOWN_EXCEPTION";
        bool foundError = false;

        for (int i = 0; i < NUM_ERRORCODES; i++)
        {
            if (error_codes[i].code == exceptionCode)
            {
                foundError = true;
                exceptionString = error_codes[i].codeName;
                exceptionResult = error_codes[i].codeResult;
                break;
            }
        }

        if (!foundError)
        {
            if (!b_IgnoreVectoredExceptions)
                Logger::TypedLog(CHN_DEBUG, "Unknown exception 0x%08x occurred at 0x%08x. Continuing.\n", exceptionCode, ExceptionInfo->ContextRecord->Eip);

            return EHR_CONTINUE;
        }

        // It's a non-fatal error, we can probably ignore it. PROBABLY.
        // In that case, let's just print some info about it to the console.

        if (exceptionResult == EHR_CONTINUE)
        {
            if (gameHasBrokeCounter <= 65000) { //Check for an absurbly high number to rule out this error EVER happening during regular gameplay. incase someone gets breakpoint spam.
                if (!b_IgnoreVectoredExceptions)
                    gameHasBrokeCounter++; // Tick once every time EHR_CONTINUE has been checked.
                    Logger::TypedLog(CHN_DEBUG, "Non-fatal exception %s occurred at 0x%08x. Continuing.\n", exceptionString, ExceptionInfo->ContextRecord->Eip);
            }
            else { // Throw message telling user game will restart without ExceptionHandler.
                GameConfig::SetValue("Logger", "ExceptionHandler", 0); // Turn off ExceptionHandler, Sadly.
                MessageBoxA(NULL, "ExceptionHandler ran into a problem.\n\nSaints Row 2 will now exit when you press ENTER, when you restart the game this will NOT happen again.", "Saints Row 2 Juiced Patch", MB_ICONERROR | MB_OK);
                exit(0); // Exit Game.
            }
            return EHR_CONTINUE;
        }

        // Prevent duplicate errors. Hopefully.
        if (b_HasErrored)
            return exceptionResult;

        b_HasErrored = true;

        // ------------------

        char errorBuffer[ERR_LENGTH];

        char moduleName[MAX_PATH] = "Unknown Module";
        DWORD64 moduleBase = SymGetModuleBase64(GetCurrentProcess(), crashAddr);

        if (moduleBase) {
            if (GetModuleFileNameA((HMODULE)moduleBase, moduleName, sizeof(moduleName)) == 0) {
                snprintf(moduleName, sizeof(moduleName), "Unknown Module");
            }
        }

        uint32_t offset = crashAddr - static_cast<uint32_t>(moduleBase);

        snprintf(errorBuffer, ERR_LENGTH,
            "!- RAN INTO A FATAL ERROR AT 0x%08x -!\n\n%s\nModule: %s\nOffset: 0x%08x \nModuleBase: 0x%08x",
            crashAddr, exceptionString, moduleName,offset,moduleBase);

        // ------------------

        // Exception access violation
        if (exceptionCode == 0xC0000005)
        {
            const char* rwMode = (record->ExceptionInformation[0]) ? "Write" : "Read";
            snprintf(errorBuffer, ERR_LENGTH, "%s (%s of 0x%08p)", errorBuffer, rwMode, record->ExceptionInformation[1]);
        }

        switch (crashAddr) {
        case 0x00ce59ac:
            strcat_s(errorBuffer, ERR_LENGTH, "\n\nSomething has gone wrong with your preload (.tbl) files!\nPlease check to see if they exist, have invalid items or have gone over the preload limit.");
            break;

        case 0x00ca0c1f:
            strcat_s(errorBuffer, ERR_LENGTH, "\n\nSaints Row 2 has run out of memory.\nLAA Patch your sr2_pc.exe to prevent this crash.");
            break;

        case 0x00C080EC:
            strcat_s(errorBuffer, ERR_LENGTH, "\n\nInvalid Bitmap Error\nA texture failed to load properly and caused the game to crash.");
            break;

        case 0x00c14a8d:
            strcat_s(errorBuffer, ERR_LENGTH, "\n\nCorrupted Input Driver.\n\nA corrupted input driver has tried to access vibration and caused the game to crash.\n\nTo fix this, turn \"ForceDisableVibration\" in reloaded.ini to 1.\n\nIf that doesn't work, vjoy is a device driver known to be problematic to SR2. If you have that installed it is recommended that you uninstall it.");
            break;

        }

        // ------------------
       // if (!crashAddr == 0x00ce59ac) {
            FormatExceptionRecords(ExceptionInfo->ContextRecord, errorBuffer);
       // }
        // ------------------

        strcat_s(errorBuffer, ERR_LENGTH, "\n\nShow this error message to someone who understands disassemblers/debuggers!");


        Logger::TypedLog(CHN_DLL, "  Wanting to show error...\n");

        // Show the error
        Logger::Error(errorBuffer);

        return exceptionResult;
    }

    LONG HandleException(_EXCEPTION_POINTERS* ExceptionInfo)
    {
        Logger::TypedLog(CHN_DLL, "EXCEPTION\n");

        ExceptionHandlerResult result = HandleException_Core(ExceptionInfo);

        switch (result)
        {
            // Terminate: Will terminate the program
        case EHR_TERMINATE:
            return EXCEPTION_EXECUTE_HANDLER;
            break;

            // Continue: Will continue and not crash
        case EHR_CONTINUE:
            return EXCEPTION_CONTINUE_EXECUTION;
            break;

            // Default: Will execute handler, AKA terminate
        default:
            return EXCEPTION_EXECUTE_HANDLER;
            break;
        }

        return EXCEPTION_EXECUTE_HANDLER;
    }

    LONG HandleVectoredException(_EXCEPTION_POINTERS* ExceptionInfo)
    {
        PEXCEPTION_RECORD record = ExceptionInfo->ExceptionRecord;
        uint32_t exceptionCode = record->ExceptionCode;

        if (!b_IgnoreVectoredExceptions)
            Logger::TypedLog(CHN_DLL, "VECTOR EXCEPTION: code %08x at 0x%08x\n", exceptionCode, ExceptionInfo->ContextRecord->Eip);

        ExceptionHandlerResult result = HandleException_Core(ExceptionInfo);

        switch (result)
        {
            // Terminate: Will terminate the program
        case EHR_TERMINATE:
            return EXCEPTION_EXECUTE_HANDLER;
            break;

            // Continue: Will continue and not crash
        case EHR_CONTINUE:
            return EXCEPTION_CONTINUE_EXECUTION;
            break;

            // Default: Will execute handler, AKA terminate
        default:
            return EXCEPTION_EXECUTE_HANDLER;
            break;
        }

        return EXCEPTION_EXECUTE_HANDLER;
    }

    void AssignHandler()
    {
        if (b_HandlerAssigned || !b_Enabled)
            return;

        b_HandlerAssigned = true;

        Logger::TypedLog(CHN_DLL, "!! Assigned error handler !!\n");

        // We set both exception handlers!
        //
        // (For some people, unhandled exception doesn't
        // seem to be enough... so we'll use a vectored
        // exception handler to pick up what it doesn't.)

        SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)HandleException);

        curVectorHandler = AddVectoredExceptionHandler(1, (PVECTORED_EXCEPTION_HANDLER)HandleVectoredException);
    }

    void UnassignHandler()
    {
        if (!b_HandlerAssigned || !b_Enabled)
            return;

        if (curVectorHandler)
        {
            RemoveVectoredExceptionHandler(curVectorHandler);
            curVectorHandler = nullptr;
        }

        b_HandlerAssigned = false;
        Logger::TypedLog(CHN_DLL, "Unassigning ExceptionHandler...\n");
    }

    void Initialize()
    {
        if (GameConfig::GetValue("Logger", "ExceptionHandler", 0))
        {
            Logger::TypedLog(CHN_DLL, "Enabling ExceptionHandler.\n");
            b_Enabled = true;
            SymInitialize(GetCurrentProcess(), NULL, TRUE); // To load module names

            if (GameConfig::GetValue("Logger", "ImmediateExceptionHandler", 0))
                AssignHandler();
        }
        else
            Logger::TypedLog(CHN_DLL, "Not using ExceptionHandler.\n");
    }

    void IgnoreVectoredExceptions(bool ignored) { b_IgnoreVectoredExceptions = ignored; } 
}
