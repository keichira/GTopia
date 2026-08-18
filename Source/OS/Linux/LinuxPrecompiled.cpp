#include "../../Utils/StringUtils.h"
#include "../OSPrecompiled.h"
#include <ctime>
#include <libgen.h>
#include <sys/random.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

string GetDateTimeAsStr()
{
    struct timeval time;
    gettimeofday(&time, NULL);

    time_t nowTime = time.tv_sec;
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&nowTime));

    return string(buf);
}

uint64 GetTick()
{
    timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

string GetProgramPath()
{
    char buf[1024] = {0};
    if (readlink("/proc/self/exe", buf, sizeof(buf) - 1) < 0)
    {
        return "";
    }

    return string(dirname(buf));
}

int32 SleepMS(uint64 ms)
{
    timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;

    return nanosleep(&ts, nullptr);
}

int32 GetRandomBytes(void* pDest, uint32 size)
{
    uint32 offset = 0;
    uint8 attm = 0;

    while (offset < size)
    {
        int32 byteSize = getrandom((uint8*)pDest + offset, size - offset, 0);

        if (byteSize <= 0)
        {
            if (byteSize < 0 && errno == EINTR)
            {
                if (++attm > RANDOM_BYTE_MAX_RETRIES)
                {
                    return -1;
                }
                continue;
            }
            return false;
        }

        offset += byteSize;
    }

    return offset;
}

bool IsFileExists(const string& path)
{
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

bool IsFolderExists(const string& path)
{
    struct stat buffer;
    return ((stat(path.c_str(), &buffer) == 0) && (buffer.st_mode & S_IFMT) == S_IFDIR);
}

string GetFileExtension(const string& file)
{
    usize index = file.find_last_of('.');
    if (index != string::npos)
    {
        return file.substr(index + 1, file.length());
    }

    return "";
}

uint32 GetSecondsFromMidnight()
{
    time_t now = time(nullptr);
    struct tm time;

    localtime_r(&now, &time);

    return time.tm_hour * 3600 + time.tm_min * 60 + time.tm_sec;
}

string GetLoadAvgString()
{
    double avg[3] = {0.0f};
    if (getloadavg(avg, 3) < 0)
    {
        return "";
    }

    return ToString(avg[0]) + " " + ToString(avg[1]) + " " + ToString(avg[2]);
}

bool CreateDir(const string& path)
{
    if (IsFolderExists(path))
        return true;

    return mkdir(path.c_str(), 0755) == 0;
}

void GetTimeLocal(tm* outTime, const time_t* timer)
{
    localtime_r(timer, outTime);
}