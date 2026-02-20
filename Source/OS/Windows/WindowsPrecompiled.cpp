#include "../OSPrecompiled.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wincrypt.h>
#include <ctime>

string GetDateTimeAsStr()
{
    time_t now = time(nullptr);

    struct tm timeInfo;
    char buf[64];
    timeInfo = *localtime(&now);

    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeInfo);
    return string(buf);
}

uint64 GetTick()
{
    return GetTickCount64();
}

// https://stackoverflow.com/questions/215963/how-do-you-properly-use-widechartomultibyte
// help me pls... need to rewrite here lol
string GetProgramPath()
{
    wchar_t buf[1024] = { 0 };

    uint32 len = GetModuleFileNameW(NULL, buf, sizeof(buf));
    if(len == 0) {
        return "";
    }

    int32 sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, buf, len, NULL, 0, NULL, NULL);

    string path(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, buf, len, path.data(), sizeNeeded, NULL, NULL);

    uint32 pos = path.find_last_of("\\/");
    if(pos != string::npos) {
        path = path.substr(0, pos);
    }

    return path;
}

int32 SleepMS(uint64 ms)
{
    ::Sleep(ms);
}

int32 GetRandomBytes(void* pDest, uint32 size) 
{
    HCRYPTPROV hProv = 0;
  
    if(!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        return -1;
    }
  
    if(!CryptGenRandom(hProv, byte_len, (BYTE*)byte_buf)) {
        CryptReleaseContext(hProv, 0);
        return -1;
    }
  
    CryptReleaseContext(hProv, 0);
    return size; // umm it says always return?
}