#pragma once

#include "../Precompiled.h"
#include "../Utils/StringUtils.h"

struct TextPacketField 
{
    uint32 hash;
    const char* value;
    uint16 size;

    std::string_view GetStringView() const
    {
        return std::string_view(value, size);
    }

    string GetString()
    {
        return string(value, size);
    }

    eToIntResult GetUInt(uint32& out)
    {
        return ToUInt(value, size, out);
    }

    eToIntResult GetInt(int32& out)
    {
        return ToInt(value, size, out);
    }

    eToIntResult GetBool(bool& out)
    {
        int32 tempVal = 0;

        eToIntResult res = ToInt(value, size, tempVal);
        if(res != TO_INT_SUCCESS)
            return res;
    
        out = (tempVal != 0); 
        return TO_INT_SUCCESS;
    }
};

template<uint8 N>
struct ParsedTextPacket 
{
    TextPacketField fields[N];
    uint8 count;

    TextPacketField* Find(uint32 hash) {
        for(uint8 i = 0; i < count; ++i) {
            if(fields[i].hash == hash) {
                return &fields[i];
            }
        }
    
        return nullptr;
    }
};

template<uint8 N>
inline void ParseTextPacket(const char* data, uint32 size, ParsedTextPacket<N>& out) 
{

    out.count = 0;
    if(!data || size == 0) 
        return;

    const char* p = data;
    const char* end = data + size;

    while(p < end && out.count < N)
    {
        while(p < end && (*p == '\n' || *p == '\r'))
            ++p;

        if(p >= end) 
            break;

        const char* nextNL = (const char*)(std::memchr(p, '\n', end - p));
        const char* lineEnd = nextNL ? nextNL : end;
        const char* lineStart = p;
        p = nextNL ? nextNL + 1 : end;

        // umm, lets keep it whatever...
        if(lineEnd > lineStart && *(lineEnd - 1) == '\r') 
            --lineEnd;

        if(lineStart < lineEnd && *lineStart == '|') 
            ++lineStart;

        if(lineEnd > lineStart && *(lineEnd - 1) == '|')
            --lineEnd;

        // force check 1 |
        const char* pipe = (const char*)(std::memchr(lineStart, '|', lineEnd - lineStart));
        if(!pipe) 
            continue;

        // ||item|4, item||5
        const char* secondPipe = (const char*)(std::memchr(pipe + 1, '|', lineEnd - (pipe + 1)));
        if(secondPipe) 
            continue; 

        const char* keyStart = lineStart;
        const char* keyEnd = pipe;
        const char* valStart = pipe + 1;
        const char* valEnd = lineEnd;

        if(valEnd > valStart && *(valEnd - 1) == '\0')
            --valEnd;

        if(keyStart >= keyEnd || valStart > valEnd) 
            continue;

        uint32 hash = HashString(keyStart, (uint32)(keyEnd - keyStart));

        out.fields[out.count++] = {
            hash,
            valStart,
            (uint16)(valEnd - valStart)
        };
    }
}