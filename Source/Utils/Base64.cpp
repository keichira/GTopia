#include "Base64.h"
#include "StringUtils.h"

// https://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c

static const int32 B64index[256] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55,
56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,
0,  0,  0, 63,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };

bool IsValidBase64Char(char c)
{
    return c == '=' || IsDigit(c) || IsAlpha(c) || c == '+' || c == '/';
}

bool Base64_Decode(void* pData, uint32 size, string& out)
{
    if(!pData || size == 0)
        return false;

    if(size % 4 != 0)
        return false;

    uint8* p = (uint8*)pData;

    uint32 pad = 0;
    if (p[size - 1] == '=') pad++;
    if (p[size - 2] == '=') pad++;

    usize L = ((size + 3) / 4 - pad) * 4;
    out.resize((size/ 4) * 3 - pad, '\0');

    for(usize i = 0, j = 0; i < L; i += 4)
    {
        unsigned char c0 = p[i];
        unsigned char c1 = p[i + 1];
        unsigned char c2 = p[i + 2];
        unsigned char c3 = p[i + 3];

        int32 n =
            (B64index[c0] << 18) |
            (B64index[c1] << 12) |
            (B64index[c2] << 6)  |
            (B64index[c3]);

        out[j++] = (n >> 16) & 0xFF;
        out[j++] = (n >> 8) & 0xFF;
        out[j++] = n & 0xFF;
    }

    if(pad)
    {
        int32 n = (B64index[p[L]] << 18) | (B64index[p[L + 1]] << 12);
        out[out.size() - 1] = (n >> 16) & 0xFF;

        if(size > L + 2 && p[L + 2] != '=')
        {
            n |= (B64index[p[L + 2]] << 6);
            out.push_back((n >> 8) & 0xFF);
        }
    }

    return true;
}
