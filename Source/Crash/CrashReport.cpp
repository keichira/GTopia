#include "CrashReport.h"
#include "../Math/Math.h"
#include "CrashContext.h"

#include <atomic>
#include <ctime>

// https://stackoverflow.com/questions/59626494/understanding-memory-order-acquire-and-memory-order-release-in-c11
// might be needed

#ifdef _WIN32
#include <windows.h>
using FileHandle = HANDLE;
static FileHandle gCrashFileHandle = INVALID_HANDLE_VALUE;
#else
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
using FileHandle = int;
static FileHandle gCrashFileHandle = -1;
#endif

static char gCrashBuffers[2][2048] = {0};
static usize gCrashStatSize[2] = {0};
static std::atomic<int32> gActiveBuffer{0};
static char gCrashFilePath[256]{};
static bool gHasLoggedCrashReport = false;

bool InitializeCrashLogFile(const char* appName, uint16 appID)
{
    if (!appName)
        return false;

    CloseCrashLogFile();
    gHasLoggedCrashReport = false;

    time_t currentTime = std::time(nullptr);
    tm timeInfo{};
    GetTimeLocal(&timeInfo, &currentTime);

    CreateDir(GetProgramPath() + "/logs");
    CreateDir(GetProgramPath() + "/logs/crash");

    if (appID > 0)
    {
        std::snprintf(gCrashFilePath, sizeof(gCrashFilePath), "logs/crash/%s-%u-%04d-%02d-%02d_%02d-%02d-%02d.log",
                      appName, appID, timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday, timeInfo.tm_hour,
                      timeInfo.tm_min, timeInfo.tm_sec);
    }
    else
    {
        std::snprintf(gCrashFilePath, sizeof(gCrashFilePath), "logs/crash/%s-%04d-%02d-%02d_%02d-%02d-%02d.log",
                      appName, timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday, timeInfo.tm_hour,
                      timeInfo.tm_min, timeInfo.tm_sec);
    }

#ifdef _WIN32
    gCrashFileHandle = CreateFileA(gCrashFilePath, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS,
                                   FILE_ATTRIBUTE_NORMAL, nullptr);
    return gCrashFileHandle != INVALID_HANDLE_VALUE;
#else
    gCrashFileHandle = open(gCrashFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    return gCrashFileHandle >= 0;
#endif
}

void CloseCrashLogFile()
{
#ifdef _WIN32
    if (gCrashFileHandle != INVALID_HANDLE_VALUE)
    {
        FlushFileBuffers(gCrashFileHandle);
        CloseHandle(gCrashFileHandle);
        gCrashFileHandle = INVALID_HANDLE_VALUE;

        if (!gHasLoggedCrashReport && gCrashFilePath[0] != '\0')
        {
            DeleteFileA(gCrashFilePath);
        }
    }
#else
    if (gCrashFileHandle >= 0)
    {
        fsync(gCrashFileHandle);
        close(gCrashFileHandle);
        gCrashFileHandle = -1;

        if (!gHasLoggedCrashReport && gCrashFilePath[0] != '\0')
        {
            unlink(gCrashFilePath);
        }
    }
#endif
}

// https://stackoverflow.com/questions/23487372/what-are-some-use-cases-for-memory-order-relaxed

void SetCrashStatsBuffer(const char* formattedStats)
{
    if (!formattedStats)
        return;

    // https://bartoszmilewski.com/2008/12/01/c-atomics-and-memory-ordering/
    // https://bartoszmilewski.com/2008/12/23/the-inscrutable-c-memory-model/
    // dayum
    int32 writeIndex = 1 - gActiveBuffer.load(std::memory_order_relaxed);

    int len = std::snprintf(gCrashBuffers[writeIndex], sizeof(gCrashBuffers[writeIndex]), "%s", formattedStats);
    gCrashStatSize[writeIndex] = (len > 0) ? Min(len, sizeof(gCrashBuffers[writeIndex]) - 1) : 0;

    gActiveBuffer.store(writeIndex, std::memory_order_release);
}

static void WriteRaw(const char* data, usize length)
{
    if (!data || length == 0)
        return;

    gHasLoggedCrashReport = true;

#ifdef _WIN32
    if (gCrashFileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD written = 0;
        WriteFile(gCrashFileHandle, data, (DWORD)(length), &written, nullptr);
        FlushFileBuffers(gCrashFileHandle);
    }
#else
    if (gCrashFileHandle >= 0)
    {
        write(gCrashFileHandle, data, length);
        fsync(gCrashFileHandle);
    }
#endif
}

void DefaultCrashCallback(const CrashInfo& info)
{
    /**
     * need to find a way to show time
     * file name shows app start time - had to open file earlier since need to becarefull when crash happened
     * not async safe
     */

    char headerBuf[512]{};
    int len = std::snprintf(headerBuf, sizeof(headerBuf),
                            "========== GTopia Crash Report ==========\n"
                            "Crash Code: 0x%08X\n"
                            "Crash Address: 0x%llX\n"
                            "Reason: %s\n"
                            "=========================================\n",
                            info.code, (unsigned long long)(info.address), info.reason ? info.reason : "Unknown");

    if (len > 0)
    {
        WriteRaw(headerBuf, len);
    }

    char contextBuf[2048]{};
    usize contextLen = CrashContext::Dump(contextBuf, sizeof(contextBuf));
    if (contextLen > 0)
    {
        WriteRaw(contextBuf, contextLen);
    }

    // messing with it was mistake bruh
    int32 readIndex = gActiveBuffer.load(std::memory_order_acquire);
    usize statsLen = gCrashStatSize[readIndex];

    if (statsLen > 0)
    {
        WriteRaw(gCrashBuffers[readIndex], statsLen);
    }
}