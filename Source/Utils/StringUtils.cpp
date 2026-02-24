#include "StringUtils.h"

uint32 HashString(const string &str)
{
    return HashString(str.c_str());
}

uint32 HashString(const char* str, uint32 size)
{
    uint32_t h = 0x12668559; 
    for (uint32 i = 0; i < size; ++i) {
        h ^= (uint8)str[i];
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

uint32 HashString(const char* str)
{
    uint32_t h = 0x12668559; 
    for (; *str; ++str) {
        h ^= (uint8)(*str);
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

string ToHex(const string& str)
{
    return ToHex(str.c_str(), str.size());
}

string ToHex(const void* str, uint32 size)
{
    static const char hexChars[] = "0123456789ABCDEF";

    const uint8* bytes = static_cast<const uint8_t*>(str);

    string out;
    out.resize(size * 2);

    for (uint32 i = 0; i < size; ++i) {
        out[2 * i]     = hexChars[(bytes[i] >> 4) & 0x0F];
        out[2 * i + 1] = hexChars[bytes[i] & 0x0F];
    }

    return out;
}

string ToUpper(const string& str)
{
    return ToUpper(str.c_str());
}

string ToUpper(const char* str)
{
    if(!str) {
        return "";
    }

    const char* src = str;
    string out;

    while(*src) {
        if(*src >= 'a' && *src <= 'z') {
            out += (*src - ('a' - 'A'));
        }
        else {
            out += *src;
        }

        ++src;
    }

    return out;
}

string ToLower(const string& str)
{
    return ToLower(str.c_str());
}

string ToLower(const char* str)
{
    if(!str) {
        return "";
    }

    const char* src = str;
    string out;

    while(*src) {
        if(*src >= 'A' && *src <= 'Z') {
            out += (*src + ('a' - 'A'));
        }
        else {
            out += *src;
        }

        ++src;
    }

    return out;
}

float ToFloat(const string& str)
{
    return ToFloat(str.c_str());
}

float ToFloat(const char* str)
{
    return std::stof(str);
}

int32 ToInt(const string& str)
{
    return int32();
}

int32 ToInt(const char* str)
{
    if(!str) {
        return 0;
    }

    return (int32)std::stol(str);
}

uint32 ToUInt(const string& str)
{
    return ToUInt(str.c_str());
}

uint32 ToUInt(const char* str)
{
    if(!str) {
        return 0;
    }

    return (uint32)std::stoul(str);
}

uint32 CountCharacter(const string& str, char character)
{
    return CountCharacter(str.c_str(), character);
}

uint32 CountCharacter(const char* str, char character)
{
    if(!str) {
        return 0;
    }

    uint32 count = 0;
    const char* src = str;

    while(*src) {
        if(*src == character) {
            count++;
        }

        ++src;
    }

    return count;
}

bool IsAlpha(char c)
{
    return isalpha(c) != 0;
}

bool IsDigit(char c)
{
    return isdigit(c) != 0;
}

void ReplaceString(string& str, const string& replaceThis, const string& replaceTo)
{
    if(str.empty()) {
        return;
    }

    int32 pos = 0;
    while((pos = str.find(replaceThis, pos)) != -1) {
        str.replace(pos, replaceThis.length(), replaceTo);
        pos += replaceTo.length();
    }
}

std::vector<string> Split(const string& str, char delim)
{
    return Split(str.c_str(), str.size(), delim);
}

std::vector<string> Split(const char* str, uint32 size, char delim)
{
    const char* strEnd = str + size;    
    const char* lastDelim = str;

    std::vector<string> token;

    for(const char* c = str; c != strEnd; ++c) {
        if(*c == delim) {
            ptrdiff_t diff = c - lastDelim;
            token.emplace_back(string(lastDelim, diff));
            lastDelim = c + 1;
        }
    }

    if(lastDelim != str) {
        ptrdiff_t diff = strEnd - lastDelim;
        token.emplace_back(string(lastDelim, diff));
    }

    return token;
}
