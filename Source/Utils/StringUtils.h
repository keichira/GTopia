#pragma once

#include "../Precompiled.h"
#include "../Math/Color.h"

constexpr uint32 CompileTimeHashString(const char* str, uint32 h = 0x12668559)
{
    return (str[0] == '\0') ? h
        : CompileTimeHashString(
            str + 1, 
            ( (h ^ static_cast<uint8>(str[0])) * 0x5bd1e995 ) ^ ( ((h ^ static_cast<uint8>(str[0])) * 0x5bd1e995) >> 15 )
        );
}

uint32 HashString(const string& str);
uint32 HashString(const char* str, uint32 size);
uint32 HashString(const char* str);

string ToHex(const string& str);
string ToHex(const void* str, uint32 size);

int32 CharToInt(char input);

void HexToBytes(const string& str, uint8* target);
void HexToBytes(const char* str, uint8* target);

string ToUpper(const string& str);
string ToUpper(const char* str);

string ToLower(const string& str);
string ToLower(const char* str);

float ToFloat(const string& str);
float ToFloat(const char* str);

void RemoveExtraWhiteSpaces(string& str);
void RemoveExtraWhiteSpaces(char* str);

string TrimLeft(const string& str, const string& trim);
string TrimRight(const string& str, const string& trim);

void StripWhiteSpace(string& str);

void RemoveGTColorCodes(string& str);
void RemoveGTColorCodes(char* str);

enum eToIntResult 
{
    TO_INT_FAIL,
    TO_INT_SUCCESS,
    TO_INT_OVERFLOW,
    TO_INT_UNDERFLOW,
};

eToIntResult ToInt(const string& str, int32& out, int32 base = 10);
eToIntResult ToInt(const char* str, int32& out, int32 base = 10);

eToIntResult ToUInt(const string& str, uint32& out, int32 base = 10);
eToIntResult ToUInt(const char* str, uint32& out, int32 base = 10);

int32 ToInt(const string& str);
int32 ToInt(const char* str);

uint32 ToUInt(const string& str);
uint32 ToUInt(const char* str);

Color ToColor(const string& str, char delim);

uint32 CountCharacter(const string& str, char character);
uint32 CountCharacter(const char* str, char character);

bool IsAlpha(char c);
bool IsDigit(char c);
bool IsUpper(char c);
bool IsLower(char c);

void ReplaceString(string& str, const string& replaceThis, const string& replaceTo);

std::vector<string> Split(const string& str, char delim);
std::vector<string> Split(const char* str, uint32 size, char delim);

std::vector<std::string_view> SplitView(const string& str, char delim);
std::vector<std::string_view> SplitView(const char* str, uint32 size, char delim);

string JoinString(const std::vector<string>& strs, const string& delim, uint32 startIdx = 0, uint32 endIdx = 0);

template<typename T>
string ToString(const T& value) { return std::to_string(value); }