#pragma once

#include "../Precompiled.h"
#include "PlayMod.h"

uint32 StrToCharacterStateFlag(const string& flag);

class PlayModManager {
public:
    PlayModManager();
    ~PlayModManager();

public:
    static PlayModManager* GetInstance()
    {
        static PlayModManager instance;
        return &instance;
    }
    
public:
    bool Load(const string& filePath);

    PlayMod* GetPlayMod(ePlayModType type);

private:
    std::vector<PlayMod> m_playMods;
};

PlayModManager* GetPlayModManager();