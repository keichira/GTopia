#pragma once
#include <stdint.h>
#include <cstddef>
#include <string>

#define RANDOM_BYTE_MAX_RETRIES 2

typedef uint64_t uint64;
typedef int64_t int64;
typedef uint32_t uint32;
typedef int32_t int32;
typedef uint16_t uint16;
typedef int16_t int16;
typedef uint8_t uint8;
typedef int8_t int8;
typedef size_t usize;
typedef std::string string;

string GetDateTimeAsStr();
uint64 GetTick();
string GetProgramPath();
int32 SleepMS(uint64 ms);
int32 GetRandomBytes(void* pDest, uint32 size);
bool IsFileExists(const string& path);