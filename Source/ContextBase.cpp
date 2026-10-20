#include "ContextBase.h"
#include <atomic>

ContextBase::ContextBase() : m_id(0), m_stopFlag(0), m_shutdownFlag(0) {}

ContextBase::~ContextBase() {}

void ContextBase::Init()
{
    m_runtimeStats.Init();
}

void ContextBase::Kill() {}

// gonna check async signal

#ifdef _WIN32
#include <chrono>
#include <windows.h>

static HANDLE sShutdownFinishedEvent = nullptr;
static std::atomic<volatile sig_atomic_t*> sShutdownFlag = nullptr;
static std::atomic<bool> sShutdownRequested = false;

static BOOL WINAPI WindowsConsoleHandler(DWORD signalType)
{
    switch (signalType)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            break;

        default:
            return FALSE;
    }

    // this thing needed, make that thing only once
    bool expected = false;
    if (!sShutdownRequested.compare_exchange_strong(expected, true, std::memory_order_acq_rel))
        return TRUE;

    volatile sig_atomic_t* shutdownFlag = sShutdownFlag.load(std::memory_order_acquire);
    if (shutdownFlag)
    {
        *shutdownFlag = 1;
    }

    SystemSignal::WaitForShutdownComplete(15);
    return TRUE;
}

void SystemSignal::RegisterShutdownHook(volatile sig_atomic_t* shutdownFlag)
{
    if (!shutdownFlag)
        return;

    sShutdownFinishedEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!sShutdownFinishedEvent)
        return;

    sShutdownFlag.store(shutdownFlag, std::memory_order_release);
    SetConsoleCtrlHandler(WindowsConsoleHandler, TRUE);
}

void SystemSignal::SignalShutdownComplete()
{
    HANDLE event = sShutdownFinishedEvent;
    if (event)
    {
        SetEvent(event); // already thread safe ig
    }
}

bool SystemSignal::WaitForShutdownComplete(uint32 timeoutSeconds)
{
    HANDLE event = sShutdownFinishedEvent;
    if (!event)
        return false;

    DWORD timeoutMs = (DWORD)(timeoutSeconds * 1000);
    return WaitForSingleObject(event, timeoutMs) == WAIT_OBJECT_0;
}

#else

static std::atomic<volatile sig_atomic_t*> sShutdownFlag{nullptr};

static void PosixSignalHandler(int signalNumber)
{
    switch (signalNumber)
    {
        case SIGINT:
        case SIGTERM:
        case SIGHUP:
        {
            volatile sig_atomic_t* shutdownFlag = sShutdownFlag.load(std::memory_order_relaxed);
            if (shutdownFlag)
            {
                *shutdownFlag = 1;
            }
            break;
        }
        default:
            break;
    }
}

void SystemSignal::RegisterShutdownHook(volatile sig_atomic_t* shutdownFlag)
{
    sShutdownFlag.store(shutdownFlag, std::memory_order_release);

    struct sigaction sa{};
    sa.sa_handler = PosixSignalHandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    sigaction(SIGINT, &sa, nullptr);
    sigaction(SIGTERM, &sa, nullptr);
    sigaction(SIGHUP, &sa, nullptr);
}

void SystemSignal::SignalShutdownComplete() {}

bool SystemSignal::WaitForShutdownComplete(uint32 timeoutSeconds)
{
    return true;
}

#endif