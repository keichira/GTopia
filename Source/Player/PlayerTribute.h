#pragma once

#include "../Precompiled.h"

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

    const PlayerTributeClientData& GetClientData() const { return m_clientData; }

private:
    PlayerTributeClientData m_clientData;
    std::vector<string> m_dataVec;
};

PlayerTribute* GetPlayerTributeManager();