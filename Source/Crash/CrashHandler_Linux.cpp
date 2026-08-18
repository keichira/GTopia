// https://stackoverflow.com/questions/17942034/simple-linux-signal-handling

#include <fcntl.h>
#include <signal.h>
#include <sys/resource.h>
#include <unistd.h>

#include "CrashHandler.h"
#include "CrashReport.h"

static uint8 gSignalStack[64 * 1024];

static void ExceptionHandler(int signalNumber, siginfo_t* signalInfo, void*)
{
    CrashInfo info{};
    info.code = (uint32)signalNumber;

    if (signalInfo)
    {
        info.address = (uint64)signalInfo->si_addr;
    }

    switch (signalNumber)
    {
        case SIGSEGV:
            info.reason = "SIGSEGV";
            break;
        case SIGABRT:
            info.reason = "SIGABRT";
            break;
        case SIGFPE:
            info.reason = "SIGFPE";
            break;
        case SIGILL:
            info.reason = "SIGILL";
            break;
        case SIGBUS:
            info.reason = "SIGBUS";
            break;
        default:
            info.reason = "Unknown Linux Signal";
            break;
    }

    DefaultCrashCallback(info);

    struct sigaction sa{};
    sa.sa_handler = SIG_DFL; // built-ib
    sigemptyset(&sa.sa_mask);
    sigaction(signalNumber, &sa, nullptr);

    kill(getpid(), signalNumber);
    _exit(128 + signalNumber);
}

// https://stackoverflow.com/questions/2919378/how-to-enable-core-dump-in-my-linux-c-program

bool InitializeCrashHandler()
{
    struct rlimit rl{}; // u limit needed?
    rl.rlim_cur = RLIM_INFINITY;
    rl.rlim_max = RLIM_INFINITY;
    setrlimit(RLIMIT_CORE, &rl);

    stack_t ss{};
    ss.ss_sp = gSignalStack;
    ss.ss_size = sizeof(gSignalStack);
    if (sigaltstack(&ss, nullptr) != 0)
        return false;

    struct sigaction sa{};
    sa.sa_sigaction = ExceptionHandler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);

    for (int signalNumber : {SIGSEGV, SIGABRT, SIGFPE, SIGILL, SIGBUS})
    {
        if (sigaction(signalNumber, &sa, nullptr) != 0)
            return false;
    }

    return true;
}