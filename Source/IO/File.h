#pragma once

#include "../Precompiled.h"

enum FileMode
{
    FILE_MODE_READ,
    FILE_MODE_WRITE,
    FILE_MODE_APPEND
};

static string fileOpenModeArr[] = { "rb", "wb", "ab" };

class File {
public:
    File();
    ~File();

public:
    bool Open(const string& filePath, FileMode mode = FILE_MODE_READ);
    void Close();

    bool IsOpen() const { return m_pFile; }
    uint32 GetSize() const { return m_size; }

    uint32 Read(void* pDest, uint32 size);
    uint32 Write(void* pData, uint32 size);
    uint32 Seek(uint32 size);

private:
    FILE* m_pFile;  
    uint32 m_size;
    uint32 m_pos;
    FileMode m_mode;
};