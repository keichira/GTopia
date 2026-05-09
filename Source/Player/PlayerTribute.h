#pragma once

#include "../Precompiled.h"
#include "../Memory/MemoryBuffer.h"

#define MAX_TRIBUTE_DATA_HEADER_COUNT 4
#define MAX_TRIBUTE_DATA_VERSION_COUNT 2

struct PlayerTributeClientData
{
    uint8* pData = nullptr;
    uint32 hash = 0;
    uint32 size = 0;
};

class PlayerTribute {
public:
    PlayerTribute();
    ~PlayerTribute();

public:
    static PlayerTribute* GetInstance()
    {
        static PlayerTribute instance;
        return &instance;
    }

public:
    bool Load(const string& filePath);
    void SaveToClientData();

    PlayerTributeClientData* GetClientData(uint32 protocol);

private:
    void Serialize(MemoryBuffer& memBuffer, uint8 version);

private:
    PlayerTributeClientData m_clientData[MAX_TRIBUTE_DATA_VERSION_COUNT];
    string m_tributeData[MAX_TRIBUTE_DATA_HEADER_COUNT];
};

PlayerTribute* GetPlayerTributeManager();