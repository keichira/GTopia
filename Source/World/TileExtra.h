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

enum eTileExtraLockFlag
{
    TILE_EXTRA_LOCK_FLAG_IGNORE_AIR = 1 << 0
};

class TileExtra_Lock : public TileExtra {
public:
    static constexpr uint8 TYPE = TILE_EXTRA_TYPE_LOCK;

    TileExtra_Lock() : TileExtra(TYPE) {}

    uint8 flags = 0;
    int32 ownerID = -1;
    std::vector<int32> accessList; // includes tempo
    int32 minEntryLevel = 0;
    int32 worldTimer = 0;

    void SetFlag(uint8 flag) { flags |= flag; }
    void RemoveFlag(uint8 flag) { flags &= ~flag; }
    bool HasFlag(uint8 flag) { return flags & flag; };

    int32 GetTempo()
    {
        for(auto& id : accessList) {
            if(id < 0) {
                return -id; // positive
            }
        }

        return -1;
    }

    void SetTempo(uint32 tempo)
    {
        for(auto& id : accessList) {
            if(id < 0) {
                id = -tempo;
                return;
            }
        }

        accessList.push_back(-tempo);
    }

    bool HasAccess(int32 userID)
    {
        if(ownerID == userID) {
            return true;
        }

        for(auto& id : accessList) {
            if(id == userID) {
                return true;
            }
        }

        return false;
    }

    bool IsAdmin(int32 userID)
    {
        for(auto& id : accessList) {
            if(id == userID && ownerID != userID) {
                return true;
            }
        }

        return false;
    }

protected:
    void Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion) override;
};