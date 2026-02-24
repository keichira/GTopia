#include "File.h"

#include "IO/Log.h"

File::File()
: m_pFile(nullptr), m_mode(FILE_MODE_READ), m_pos(0), m_size(0)
{
}

File::~File()
{
    Close();
}

bool File::Open(const string& filePath, FileMode mode)
{       
    m_mode = mode;
    m_pFile = fopen(filePath.c_str(), fileOpenModeArr[mode].c_str());
    if(!m_pFile) {
        Close();
        return false;
    }

    fseek(m_pFile, 0, SEEK_END);
    m_size = ftell(m_pFile);
    m_pos  = m_size;

    if(mode == FILE_MODE_READ) {
        fseek(m_pFile, 0, SEEK_SET);
        m_pos = 0;
    }

    return true;
}

void File::Close()
{
    if(m_pFile) {
        fclose(m_pFile);
        m_pFile = nullptr;
    }
}

uint32 File::Read(void* pDest, uint32 size)
{
    if(!IsOpen()) {
        return 0;
    }

    if(m_mode != FILE_MODE_READ) {
        return 0;
    }

    if(size + m_pos > m_size) {
        return 0;
    }

    uint32 sizeRead = fread(pDest, 1, size, m_pFile);
    m_pos += sizeRead;

    return sizeRead;
}

uint32 File::Write(void* pData, uint32 size)
{
    if(!IsOpen()) {
        return 0;
    }

    if(m_mode != FILE_MODE_WRITE && m_mode != FILE_MODE_APPEND) {
        return 0;
    }

    uint32 sizeWrite = fwrite(pData, 1, size, m_pFile);
    if(sizeWrite != size) {
        //fseek(m_pFile, m_pos, SEEK_SET);
        return 0;
    }
    //fflush(m_pFile);

    m_pos += size;
    m_size += size;
    return size;
}

uint32 File::Seek(uint32 size)
{
    if(!IsOpen()) {
        return 0;
    }

    if(m_mode == FILE_MODE_READ && m_pos + size > m_size) {
        return 0;
    }

    m_pos += size;
    fseek(m_pFile, m_pos, SEEK_SET);
    return m_pos;
}
