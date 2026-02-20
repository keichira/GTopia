#include "../OSPrecompiled.h"
#include <sys/time.h>
#include <ctime>
#include <unistd.h>
#include <libgen.h>
#include <sys/random.h>

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
    char buf[1024] = { 0 };
    if(readlink("/proc/self/exe", buf, sizeof(buf) - 1) < 0) {
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

    while(offset < size) {
        int32 byteSize = getrandom((uint8*)pDest + offset, size - offset, 0);
        
        if(byteSize <= 0) {
            if(byteSize < 0 && errno == EINTR) {
                if(++attm > RANDOM_BYTE_MAX_RETRIES) {
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