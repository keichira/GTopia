#pragma once

#include "../Precompiled.h"
#include "../Utils/StringUtils.h"

struct TextPacketField 
{
    uint32 hash;
    const char* value;
    uint16 size;

    std::string_view GetString() const
    {
        return std::string_view(value, size);
    }
};

template<uint8 N>
struct ParsedTextPacket 
{
    TextPacketField fields[N];
    uint8 count;

    const TextPacketField* Find(uint32 hash) {
        for(uint8 i = 0; i < count; ++i) {
            if(fields[i].hash == hash) {
                return &fields[i];
            }
        }
    
        return nullptr;
    }
};

#include "../IO/Log.h"

template<uint8 N>
inline void ParseTextPacket(const char* data, uint32 size, ParsedTextPacket<N>& out) 
{
    out.count = 0;

    const char* curr = data;
    const char* strEnd = data + size;

    while(curr < strEnd && out.count < N) {
        const char* delim = nullptr;
        const char* fieldEnd = nullptr;

        /**
         * fixing the input packet by that start var
         * |text|
         * it skips the delim '|' if index 0 is '|'
         */

        const char* start = (curr < strEnd && *curr == '|') ? curr + 1 : curr;
        for(const char* c = start; c < strEnd; ++c) {
            if(!delim && *c == '|') {
                delim = c;
            }
            if(*c == '\n') {
                fieldEnd = c;
                break;
            }
        }

        if(!fieldEnd) {
            fieldEnd = strEnd;
        }

        if(delim) {
            uint32 hash = HashString(start, delim - start);

            const char* valEnd = fieldEnd;
            if(valEnd > delim + 1 && *(valEnd - 1) == '\r') {
                --valEnd;
            }

            out.fields[out.count++] = TextPacketField{ hash, delim + 1, (uint16)(valEnd - (delim + 1)) };
        }

        curr = (fieldEnd < strEnd) ? fieldEnd + 1 : strEnd;
    }
}