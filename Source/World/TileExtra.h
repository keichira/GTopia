#pragma once

#include "../Precompiled.h"
#include "../Memory/MemoryBuffer.h"

enum eTileExtraTypes 
{
    TILE_EXTRA_TYPE_NONE = 0,
    TILE_EXTRA_TYPE_DOOR = 1,

    TILE_EXTRA_TYPE_SIZE
};

class TileInfo;

class TileExtra {
public:
    TileExtra();

public:
    static bool HasExtra(uint8 itemType);

public:
    bool Setup(uint8 itemType);
    bool Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile);

    string GetName() const { return m_name; }
    string GetText() const  { return m_text; }
    string GetID() const { return m_id; }

private:
    uint8 m_type;
    string m_name;
    string m_text;
    string m_id;
    int32 m_ownerID;
};