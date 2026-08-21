#include "CrashHandler.h"
#include <Windows.h>

static LONG WINAPI ExceptionHandler(EXCEPTION_POINTERS* exceptionInfo)
{
    CrashInfo info{};

    if (exceptionInfo && exceptionInfo->ExceptionRecord)
    {
        DWORD code = exceptionInfo->ExceptionRecord->ExceptionCode;

        // if (code == STATUS_CONTROL_C_EXIT || code == DBG_CONTROL_C) // check dunno if it work
        //     return EXCEPTION_CONTINUE_SEARCH;

        info.code = (uint32)code;
        info.address = (uintptr_t)(exceptionInfo->ExceptionRecord->ExceptionAddress);

        switch (info.code)
        {
            case EXCEPTION_ACCESS_VIOLATION:
                info.reason = "EXCEPTION_ACCESS_VIOLATION";
                break;

            case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
                info.reason = "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
                break;

            case EXCEPTION_BREAKPOINT:
                info.reason = "EXCEPTION_BREAKPOINT";
                break;

            case EXCEPTION_DATATYPE_MISALIGNMENT:
                info.reason = "EXCEPTION_DATATYPE_MISALIGNMENT";
                break;

            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                info.reason = "EXCEPTION_FLT_DIVIDE_BY_ZERO";
                break;

            case EXCEPTION_FLT_OVERFLOW:
                info.reason = "EXCEPTION_FLT_OVERFLOW";
                break;

            case EXCEPTION_FLT_UNDERFLOW:
                info.reason = "EXCEPTION_FLT_UNDERFLOW";
                break;

            case EXCEPTION_INT_DIVIDE_BY_ZERO:
                info.reason = "EXCEPTION_INT_DIVIDE_BY_ZERO";
                break;

            case EXCEPTION_INT_OVERFLOW:
                info.reason = "EXCEPTION_INT_OVERFLOW";
                break;

            case EXCEPTION_ILLEGAL_INSTRUCTION:
                info.reason = "EXCEPTION_ILLEGAL_INSTRUCTION";
                break;

            case EXCEPTION_IN_PAGE_ERROR:
                info.reason = "EXCEPTION_IN_PAGE_ERROR";
                break;

            case EXCEPTION_STACK_OVERFLOW:
                info.reason = "EXCEPTION_STACK_OVERFLOW";
                break;

            case EXCEPTION_NONCONTINUABLE_EXCEPTION:
                info.reason = "EXCEPTION_NONCONTINUABLE_EXCEPTION";
                break;

            default:
                info.reason = "Unknown Windows exception";
                break;
        }
    }
    else
    {
        info.reason = "Unknown Windows exception";
    }

    CrashCallback callback = GetCrashCallback();

    if (callback) // old
        callback(info);

    return EXCEPTION_EXECUTE_HANDLER;
}

bool InitializeCrashHandler()
{
    SetUnhandledExceptionFilter(ExceptionHandler);
    return true;
}