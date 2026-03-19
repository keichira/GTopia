#include "StringUtils.h"

uint32 HashString(const string& str)
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

int32 CharToInt(char input)
{
    if(input >= '0' && input <= '9') {
        return input - '0';
    }

    if(input >= 'A' && input <= 'F') {
        return input - 'A' + 10;
    }

    if(input >= 'a' && input <= 'f') {
        return input - 'a' + 10;
    }

    return 0;
}

void HexToBytes(const string& str, uint8* target)
{
    HexToBytes(str.c_str(), target);
}

void HexToBytes(const char *str, uint8 *target)
{
    if(!str || !target) {
        return;
    }

    const char* src = str;

    while(*src && *(src + 1)) {
        *(target++) = CharToInt(*src) * 16 + CharToInt(src[1]);
        src += 2;
    }
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

void RemoveExtraWhiteSpaces(string& str)
{
    RemoveExtraWhiteSpaces(&str[0]);

    str.resize(strlen(&str[0]));
}

void RemoveExtraWhiteSpaces(char *str)
{
    if(!str) {
        return;
    }

    char* src = str;
    char* out = str;

    if(*src == ' ') {
        *out++ = ' ';

        while(*src == ' ') {
            src++;
        }
    }

    while(*src) {
        if (*src != ' ' || (out > str && *(out - 1) != ' ')) {
            *out++ = *src;
        }

        src++;
    }

    if(out > str && *(out - 1) == ' ') {
        out--;
    }

    *out = '\0';

}

void RemoveAllSpaces(string& str)
{
    RemoveAllSpaces(&str[0]);

    str.resize(strlen(&str[0]));
}

void RemoveAllSpaces(char* str)
{
    if(!str) {
        return;
    }

    char* src = str;
    char* out = str;

    while(*src) {
        if(*src != ' ') {
            *out++ = *src;
        }

        src++;
    }

    *out = '\0';
}

eToIntResult ToInt(const string& str, int32& out, int32 base)
{
    return ToInt(str.c_str(), out, base);
}

eToIntResult ToInt(const char* str, int32& out, int32 base)
{
    if(!str) {
        return TO_INT_FAIL;
    }

    char* end;
    errno = 0;
    long val = std::strtol(str, &end, base);

    if(*str == '\0') {
        return TO_INT_FAIL;
    }

    if(errno == ERANGE || val > INT32_MAX) {
        return TO_INT_OVERFLOW;
    }

    if(errno == ERANGE || val < INT32_MIN) {
        return TO_INT_UNDERFLOW;
    }

    out = (int32)val;
    return TO_INT_SUCCESS;
}

eToIntResult ToUInt(const string& str, uint32& out, int32 base)
{
    return ToUInt(str.c_str(), out, base);
}

eToIntResult ToUInt(const char* str, uint32& out, int32 base)
{
    if(!str) {
        return TO_INT_FAIL;
    }
    
    if(*str == '-') {
        return TO_INT_UNDERFLOW;
    }

    char* end;
    errno = 0;
    unsigned long val = std::strtoul(str, &end, base);

    if(errno == ERANGE || val > UINT32_MAX) {
        return TO_INT_OVERFLOW;
    }
    
    if(*str == '\0' || *end != '\0') {
        return TO_INT_FAIL;
    }

    out = (uint32)val;
    return TO_INT_SUCCESS;
}

Color ToColor(const string& str, char delim)
{
    string removedStr = str;
    RemoveAllSpaces(removedStr);

    auto args = Split(removedStr, delim);
    if(args.size() < 3) {
        return Color::New;
    }

    uint32 r = 255;
    if(ToUInt(args[0], r) != TO_INT_SUCCESS) {
        return Color::New;
    }

    uint32 g = 255;
    if(ToUInt(args[1], g) != TO_INT_SUCCESS) {
        return Color::New;
    }

    uint32 b = 255;
    if(ToUInt(args[2], b) != TO_INT_SUCCESS) {
        return Color::New;
    }

    Color color;
    color.r = (uint8)r;
    color.g = (uint8)g;
    color.b = (uint8)b;

    if(args.size() > 3) {
        uint32 a = 255;
        if(ToUInt(args[3], a) != TO_INT_SUCCESS) {
            return Color::New;
        }

        color.a = (uint8)a;
    }

    return color;
}

int32 ToInt(const string& str)
{
    return ToInt(str.c_str());
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

    if(lastDelim != strEnd) {
        ptrdiff_t diff = strEnd - lastDelim;
        token.emplace_back(string(lastDelim, diff));
    }

    return token;
}

string JoinString(const std::vector<string>& strs, const string& delim, uint32 startIdx, uint32 endIdx)
{
    if(strs.empty()) {
        return "";
    }

    if(endIdx == 0 || endIdx > strs.size()) {
        endIdx = strs.size();
    }

    string ret;
    for(uint32 i = startIdx; i < endIdx; ++i) {
        if(i > startIdx) {
            ret += delim;
        }
        
        ret += strs[i];
    }

    return ret;
}
