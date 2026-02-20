#pragma once

#include "../Precompiled.h"

constexpr uint32 CompileTimeHashString(const char* str, uint32 h = 0x12668559)
{
    return (str[0] == '\0') ? h : CompileTimeHashString(
        str + 1, 
        ((h ^ static_cast<uint8_t>(str[0])) * 0x5bd1e995) ^ (((h ^ static_cast<uint8_t>(str[0])) * 0x5bd1e995) >> 15)
    );
}

uint32 HashString(const string& str);
uint32 HashString(const char* str, uint32 size);
uint32 HashString(const char* str);

string ToHex(const string& str);
string ToHex(const void* str, uint32 size);

void ToUpper(string& str);
void ToUpper(char* str);

void ToLower(string& str);

float ToFloat(const string& str);
float ToFloat(const char* str);

int32 ToInt(const string& str);
int32 ToInt(const char* str);

uint32 ToUInt(const string& str);
uint32 ToUInt(const char* str);

uint32 CountCharacter(const string& str, char character);
uint32 CountCharacter(const char* str, char character);

std::vector<string> Split(const string& str, char delim);
std::vector<string> Split(const char* str, uint32 size, char delim);

template<typename T>
string ToString(const T& value) { return std::to_string(value); }