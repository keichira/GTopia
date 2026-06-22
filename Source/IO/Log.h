#pragma once

#include "../Precompiled.h"
#include <concurrentqueue.h>

#define DEFAULT_LOG_WRITE_COUNT 100

class File;

enum eLogLevel
{
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG
};

static string logLevelArr[] = { "WARN", "ERROR", "INFO", "DEBUG" };

class Log {
public:
    Log();
    ~Log();

public:
    static Log* GetInstance()
    {
        static Log instance;
        return &instance;
    }

public:
    bool InitFile(const string& filePath);
    void Print(eLogLevel logLvl, bool asapWrite, const char* fmt, ...);
    uint32 Write(uint32 count = DEFAULT_LOG_WRITE_COUNT);
    uint32 Flush();

    void Kill();

private:
    File* m_pFile;
    moodycamel::ConcurrentQueue<string> m_logs;
};

Log* GetLog();

#define LOGGER_LOG_BASE(lvl, asap, fmt, ...) { GetLog()->Print(lvl, asap, fmt, ##__VA_ARGS__); }
#define LOGGER_LOG_WARN(fmt, ...) LOGGER_LOG_BASE(eLogLevel::LOG_LEVEL_WARN, false, fmt, ##__VA_ARGS__)
#define LOGGER_LOG_ERROR(fmt, ...) LOGGER_LOG_BASE(eLogLevel::LOG_LEVEL_ERROR, false, fmt, ##__VA_ARGS__)
#define LOGGER_LOG_INFO(fmt, ...) LOGGER_LOG_BASE(eLogLevel::LOG_LEVEL_INFO, false, fmt, ##__VA_ARGS__)

#define LOGGER_LOG_WARN_ASAP(fmt, ...) LOGGER_LOG_BASE(eLogLevel::LOG_LEVEL_WARN, true, fmt, ##__VA_ARGS__)
#define LOGGER_LOG_ERROR_ASAP(fmt, ...) LOGGER_LOG_BASE(eLogLevel::LOG_LEVEL_ERROR, true, fmt, ##__VA_ARGS__)
#define LOGGER_LOG_INFO_ASAP(fmt, ...) LOGGER_LOG_BASE(eLogLevel::LOG_LEVEL_INFO, true, fmt, ##__VA_ARGS__)

#ifdef _DEBUG
    #define LOGGER_LOG_DEBUG(fmt, ...) LOGGER_LOG_BASE(eLogLevel::LOG_LEVEL_DEBUG, false, fmt, ##__VA_ARGS__)
    #define LOGGER_LOG_DEBUG_ASAP(fmt, ...) LOGGER_LOG_BASE(eLogLevel::LOG_LEVEL_DEBUG, true, fmt, ##__VA_ARGS__)
#else
    #define LOGGER_LOG_DEBUG(fmt, ...) (void(0))
    #define LOGGER_LOG_DEBUG_ASAP(fmt, ...) (void(0))
#endif