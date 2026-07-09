#include "../OSPrecompiled.h"

// clang-format off
#include <ctime>
#include <windows.h>
#include <wincrypt.h>
// clang-format on

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

string GetProgramPath()
{
    wchar_t buf[1024] = {0};

    uint32 len = GetModuleFileNameW(NULL, buf, 1024);
    if (len == 0)
        return "";

    string path = UTF16ToUTF8(std::wstring(buf, len));

    usize pos = path.find_last_of("\\/");
    if (pos != string::npos)
    {
        path = path.substr(0, pos);
    }

    return path;
}

int32 SleepMS(uint64 ms)
{
    ::Sleep(ms);
    return 0;
}

int32 GetRandomBytes(void* pDest, uint32 size)
{
    HCRYPTPROV hProv = 0;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        return -1;
    }

    if (!CryptGenRandom(hProv, size, (BYTE*)pDest))
    {
        CryptReleaseContext(hProv, 0);
        return -1;
    }

    CryptReleaseContext(hProv, 0);
    return size; // umm it says always return?
}

bool IsFileExists(const string& path)
{
    if (path.empty())
        return false;

    DWORD attr = GetFileAttributesW(UTF8ToUTF16(path).c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || (attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        return false;
    }

    return true;
}

bool IsFolderExists(const string& path)
{
    if (path.empty())
        return false;

    DWORD attr = GetFileAttributesW(UTF8ToUTF16(path).c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
    {
        return false;
    }

    return true;
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

    localtime_s(&time, &now);

    return time.tm_hour * 3600 + time.tm_min * 60 + time.tm_sec;
}

string GetLoadAvgString()
{
    return "";
}

std::wstring UTF8ToUTF16(const string& str)
{
    if (str.empty())
        return std::wstring();

    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, NULL, 0);
    if (size == 0)
        return false;

    std::wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);

    return result;
}

string UTF16ToUTF8(const std::wstring& wstr)
{
    if (wstr.empty())
        return std::string();

    int targetLen = ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    if (targetLen <= 0)
        return std::string();

    string str(targetLen, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), &str[0], targetLen, nullptr, nullptr);

    return str;
}