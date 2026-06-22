#include "Base64.h"
#include "StringUtils.h"

// https://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c

static const int32 B64index[256] = 
{
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62,  0,  0,  0, 63,
   52, 53, 54, 55, 56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,
    0,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
   15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,  0,  0,  0,  0,
    0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
   41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,  0,  0,  0,  0,  0,
};

static const char B64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

bool IsValidBase64Char(char c)
{
    return c == '=' || IsDigit(c) || IsAlpha(c) || c == '+' || c == '/';
}

bool Base64_Decode(void* pData, uint32 size, string& out)
{
    if(!pData || size == 0 || size % 4 != 0)
        return false;

    uint8* p = (uint8*)pData;

    int32 padCount = 0;
    if(p[size - 1] == '=') padCount++;
    if(p[size - 2] == '=') padCount++;

    usize expectedOutputSize = (size / 4) * 3 - padCount;
    out.resize(expectedOutputSize);

    usize L = size - 4;
    usize j = 0, i = 0;

    for(; i < L; i += 4)
    {
        uint32 n =
            (B64index[p[i]]   << 18) |
            (B64index[p[i+1]] << 12) |
            (B64index[p[i+2]] << 6) |
            (B64index[p[i+3]]);

        out[j++] = (n >> 16) & 0xFF;
        out[j++] = (n >> 8)  & 0xFF;
        out[j++] = n & 0xFF;
    }

    uint32 n = (B64index[p[i]] << 18) | (B64index[p[i + 1]] << 12);
    
    if(padCount == 2) 
    {
        out[j++] = (n >> 16) & 0xFF;
    }
    else if(padCount == 1) 
    {
        n |= (B64index[p[i + 2]] << 6);
        out[j++] = (n >> 16) & 0xFF;
        out[j++] = (n >> 8)  & 0xFF;
    }
    else 
    {
        n |= (B64index[p[i + 2]] << 6) | B64index[p[i + 3]];
        out[j++] = (n >> 16) & 0xFF;
        out[j++] = (n >> 8)  & 0xFF;
        out[j++] = n         & 0xFF;
    }

    return true;
}

bool Base64_Encode(void* pData, uint32 size, string& out)
{
    if(!pData || size == 0)
        return false;

    uint8* p = (uint8*)pData;

    usize expectedSize = ((size + 2) / 3) * 4;
    out.resize(expectedSize);

    usize i = 0, j = 0;

    while(i + 2 < size)
    {
        uint32 n = (p[i] << 16) | (p[i + 1] << 8) | p[i + 2];

        out[j++] = B64chars[(n >> 18) & 0x3F];
        out[j++] = B64chars[(n >> 12) & 0x3F];
        out[j++] = B64chars[(n >> 6)  & 0x3F];
        out[j++] = B64chars[n & 0x3F];

        i += 3;
    }

    if(i < size)
    {
        if(size - i == 1)
        {
            uint32 n = (p[i] << 16);
            out[j++] = B64chars[(n >> 18) & 0x3F];
            out[j++] = B64chars[(n >> 12) & 0x3F];
            out[j++] = '=';
            out[j++] = '=';
        }
        else if(size - i == 2)
        {
            uint32 n = (p[i] << 16) | (p[i + 1] << 8);
            out[j++] = B64chars[(n >> 18) & 0x3F];
            out[j++] = B64chars[(n >> 12) & 0x3F];
            out[j++] = B64chars[(n >> 6)  & 0x3F];
            out[j++] = '=';
        }
    }

    return true;
}
