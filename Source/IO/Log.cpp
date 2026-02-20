#include "Log.h"
#include "File.h"
#include "../Utils/Timer.h"
#include <cstdarg>

Log::Log()
: m_pFile(nullptr)
{
}

Log::~Log()
{
    Kill();
}

bool Log::InitFile(const string& filePath)
{
    if (m_pFile) {
        SAFE_DELETE(m_pFile);
    }

    m_pFile = new File();

    if (!m_pFile->Open(filePath, FILE_MODE_APPEND)) {
        SAFE_DELETE(m_pFile);
        return false;
    }
    return true;
}

void Log::Print(eLogLevel logLvl, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    printf("%s\r\n", buffer);

    if(m_pFile) {
        string str = logLevelArr[logLvl] + " | " + Time::GetDateTimeStr() + " | " + buffer + "\r\n";
        m_logs.enqueue(std::move(str));
    }
}

uint32 Log::Write(uint32 count)
{
    if(!m_pFile || count == 0) {
        return 0;
    }

    std::vector<string> batch(count > 1 ? count : DEFAULT_LOG_WRITE_COUNT);
    moodycamel::ConsumerToken token(m_logs);

    uint32 queueCount = m_logs.try_dequeue_bulk(token, batch.data(), batch.size());

    if(queueCount > 0) {
        for(uint32 i = 0; i < queueCount; ++i) {
            if(!batch[i].empty()) {
                m_pFile->Write((void*)batch[i].c_str(), batch[i].size());
            }
        }
    }

    return queueCount;
}

uint32 Log::Flush()
{
    if(!m_pFile) {
        return 0;
    }

    uint32 flushed = 0;

    while(true) {
        std::vector<string> batch(DEFAULT_LOG_WRITE_COUNT);
        moodycamel::ConsumerToken token(m_logs);

        uint32 count = m_logs.try_dequeue_bulk(token, batch.data(), batch.size());

        if(count == 0) {
            break;
        }

        for(uint32 i = 0; i < count; ++i) {
            if(!batch[i].empty()) {
                m_pFile->Write((void*)batch[i].c_str(), batch[i].size());
            }
        }

        flushed += count;
    }

    return flushed;
}

void Log::Kill()
{
    SAFE_DELETE(m_pFile);
}

Log* GetLog() { return Log::GetInstance(); }