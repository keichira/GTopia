#pragma once

#include "../Precompiled.h"
#include "../Memory/MemoryBuffer.h"

enum eTileExtraTypes 
{
    TILE_EXTRA_TYPE_NONE = 0,
    TILE_EXTRA_TYPE_DOOR = 1,
    TILE_EXTRA_TYPE_SIGN = 2,
    TILE_EXTRA_TYPE_LOCK = 3,

    TILE_EXTRA_TYPE_SIZE
};

uint8 GetTileExtraType(uint8 itemType);

class TileInfo;

class TileExtra {
public:
    explicit TileExtra(uint8 tileExtraType) : type(tileExtraType) {}
    virtual ~TileExtra() {};

    virtual void Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion) = 0;
    static TileExtra* Create(uint8 tileExtraType);

protected:
    virtual void Serialize(MemoryBuffer& memBuffer, bool write);

public:
    uint8 type = TILE_EXTRA_TYPE_NONE;
};

class TileExtra_Door : public TileExtra {
public:
    static constexpr uint8 TYPE = TILE_EXTRA_TYPE_DOOR;

    TileExtra_Door() : TileExtra(TYPE) {}

    string name;
    string text;
    string id;

protected:
    void Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion) override;
};

class TileExtra_Sign : public TileExtra {
public:
    static constexpr uint8 TYPE = TILE_EXTRA_TYPE_SIGN;

    TileExtra_Sign() : TileExtra(TYPE) {}

    string text;

protected:
    void Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion) override;
};