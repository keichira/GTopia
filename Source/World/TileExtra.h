#pragma once

#include "../Precompiled.h"
#include "../Memory/MemoryBuffer.h"
#include "Utils/Timer.h"
#include "../Item/ItemInfoManager.h"

class TileExtra;
class TileExtraGrowth;

#define TILE_EXTRA(CLASS, TYPEID)        \
class CLASS : public TileExtra {         \
public:                                  \
    static constexpr uint8 TYPE = TYPEID;\
    CLASS() : TileExtra(TYPE) {}         \
protected:                               \
    void Serialize(                      \
        MemoryBuffer& memBuffer,         \
        bool write,                      \
        bool database,                   \
        TileInfo* pTile,                 \
        uint16 worldVersion              \
    ) override;                          \
public:

#define TILE_EXTRA_GROWTH(CLASS, TYPEID) \
class CLASS : public TileExtraGrowth {  \
public:                                  \
    static constexpr uint8 TYPE = TYPEID; \
    CLASS() : TileExtraGrowth(TYPE) {}   \
protected:                               \
    void Serialize(                      \
        MemoryBuffer& memBuffer,         \
        bool write,                      \
        bool database,                   \
        TileInfo* pTile,                 \
        uint16 worldVersion              \
    ) override;                          \
public:

enum eTileExtraTypes 
{
    TILE_EXTRA_TYPE_NONE = 0,
    TILE_EXTRA_TYPE_DOOR = 1,
    TILE_EXTRA_TYPE_SIGN = 2,
    TILE_EXTRA_TYPE_LOCK = 3,
    TILE_EXTRA_TYPE_SEED = 4,
    TILE_EXTRA_TYPE_RACE_FLAG = 5,
    TILE_EXTRA_TYPE_MAILBOX = 6,
    TILE_EXTRA_TYPE_BULLETIN = 7,
    TILE_EXTRA_TYPE_COMPONENT = 8,
    TILE_EXTRA_TYPE_PROVIDER = 9,
    TILE_EXTRA_TYPE_ACHIEVEMENT = 10,
    TILE_EXTRA_TYPE_HEART_MONITOR = 11,
    TILE_EXTRA_TYPE_DONATION_BOX = 12,
    TILE_EXTRA_TYPE_TOYBOX = 13,
    TILE_EXTRA_TYPE_MANNEQUIN = 14,
    TILE_EXTRA_TYPE_MAGIC_EGG = 15,
    TILE_EXTRA_TYPE_TEAM = 16,
    TILE_EXTRA_TYPE_GAME_GEN = 17,
    TILE_EXTRA_TYPE_XENONITE = 18,
    TILE_EXTRA_TYPE_DRESSUP = 19,
    TILE_EXTRA_TYPE_CRYSTAL = 20,
    TILE_EXTRA_TYPE_BURGLAR = 21,
    TILE_EXTRA_TYPE_SPOTLIGHT = 22,
    TILE_EXTRA_TYPE_DISPLAY_BLOCK = 23,
    TILE_EXTRA_TYPE_VENDING = 24,
    TILE_EXTRA_TYPE_FISH_TANK = 25,
    TILE_EXTRA_TYPE_SOLAR = 26,
    TILE_EXTRA_TYPE_FORGE = 27,
    TILE_EXTRA_TYPE_GIVING_TREE = 28,
    TILE_EXTRA_TYPE_GIVING_TREE_STUMP = 29,
    TILE_EXTRA_TYPE_STEAM_ORGAN = 30,
    TILE_EXTRA_TYPE_TAMAGOTCHI = 31,
    TILE_EXTRA_TYPE_SEWING = 32,
    TILE_EXTRA_TYPE_FLAG = 33,
    TILE_EXTRA_TYPE_LOBSTER_TRAP = 34,
    TILE_EXTRA_TYPE_ARTCANVAS = 35,
    TILE_EXTRA_TYPE_BATTLE_CAGE = 36,
    TILE_EXTRA_TYPE_PET_TRAINER = 37,
    TILE_EXTRA_TYPE_STEAM_ENGINE = 38,
    TILE_EXTRA_TYPE_LOCK_BOT = 39,
    TILE_EXTRA_TYPE_WEATHER_SPECIAL = 40,
    TILE_EXTRA_TYPE_SPIRIT_STORAGE = 41,
    TILE_EXTRA_TYPE_BEDROCK = 42,
    TILE_EXTRA_TYPE_DISPLAY_SHELF = 43,
    TILE_EXTRA_TYPE_VIP_DOOR = 44,
    TILE_EXTRA_TYPE_CHAL_TIMER = 45,
    TILE_EXTRA_TYPE_CHAL_FLAG = 46,
    TILE_EXTRA_TYPE_FISH_MOUNT = 47,
    TILE_EXTRA_TYPE_PORTRAIT = 48,
    TILE_EXTRA_TYPE_WEATHER_SPECIAL2 = 49,
    TILE_EXTRA_TYPE_FOSSIL_PREP = 50,
    TILE_EXTRA_TYPE_DNA_MACHINE = 51,
    TILE_EXTRA_TYPE_BLASTER = 52,
    TILE_EXTRA_TYPE_CHEMTANK = 53,
    TILE_EXTRA_TYPE_STORAGE = 54,
    TILE_EXTRA_TYPE_OVEN = 55,
    TILE_EXTRA_TYPE_SUPER_MUSIC = 56,
    TILE_EXTRA_TYPE_GEIGER_CHARGER = 57,

    TILE_EXTRA_TYPE_FIELD_NODE = 67,
    TILE_EXTRA_TYPE_OUIJA_BOARD = 68,

    TILE_EXTRA_TYPE_SIZE
};

enum eTileExtraFlags
{
    TILE_EXTRA_LOCK_IGNORE_EMPTY = 1 << 0,
    TILE_EXTRA_LOCK_DISABLE_MUSIC = 1 << 4,
    TILE_EXTRA_LOCK_DISABLE_RENDER_MUSIC = 1 << 5,
    TILE_EXTRA_LOCK_SILENCE = 1 << 6,
    TILE_EXTRA_LOCK_BUILD_ONLY = 1 << 6,
    TILE_EXTRA_LOCK_RAINBOW_TRAIL = 1 << 7,
    TILE_EXTRA_LOCK_LIMIT_ADMINS = 1 << 7,

    TILE_EXTRA_CAMERA_SHOW_LEAVES = 1 << 0,
    TILE_EXTRA_CAMERA_SHOW_DROPS = 1 << 1,
    TILE_EXTRA_CAMERA_SHOW_VENDINGS = 1 << 2,
    TILE_EXTRA_CAMERA_SHOW_JOINS = 1 << 3,
    TILE_EXTRA_CAMERA_SHOW_COLLECTS = 1 << 4,
    TILE_EXTRA_CAMERA_DONT_SHOW_OTHERS = 1 << 5,
    TILE_EXTRA_CAMERA_DONT_SHOW_OWNER = 1 << 6,
    TILE_EXTRA_CAMERA_DONT_SHOW_ADMINS = 1 << 7
};

uint8 GetTileExtraType(uint8 itemType);
TileExtra* CreateTileExtra(uint8 extraType);

class TileInfo;

class TileExtra {
public:
    explicit TileExtra(uint8 tileExtraType) : type(tileExtraType) {}
    virtual ~TileExtra() {};

    virtual void Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion) = 0;
    virtual float GetGrowthPercent(TileInfo* pTile) { return 0.0f; }
    virtual void FinalizeGrowth(uint32 ageMS) {}
    virtual void ModGrowth(int32 deltaAgeSec, int32 ageSec) {}

protected:
    virtual void Serialize(MemoryBuffer& memBuffer, bool write);

public:
    uint8 type = TILE_EXTRA_TYPE_NONE;
};

class TileExtraGrowth : public TileExtra {
public:
    explicit TileExtraGrowth(uint8 tileExtraType) : TileExtra(tileExtraType)
    {
        timer = Time::GetSystemTime();
    }

    uint32 timer = Time::GetSystemTime();
    uint32 growTime = 0;

    void FinalizeGrowth(uint32 ageMS) override;
    void ModGrowth(int32 deltaAgeSec, int32 ageSec) override;
    float GetGrowthPercent(TileInfo* pTile) override;
    void Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion) override = 0;
};

TILE_EXTRA(TileExtra_Door, TILE_EXTRA_TYPE_DOOR)
    string name;
    string text;
    string id;
};

TILE_EXTRA(TileExtra_Sign, TILE_EXTRA_TYPE_SIGN)
    string text;
};

TILE_EXTRA(TileExtra_Lock, TILE_EXTRA_TYPE_LOCK)
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

    uint32 GetTotalAccessedCount()
    {
        uint32 count = 0;

        for(auto& id : accessList)
        {
            if(id > 0)
                count++;
        }

        return count;
    }

    void RemoveFromList(int32 id)
    {
        accessList.erase(accessList.begin() + id);
    }
};

TILE_EXTRA_GROWTH(TileExtra_Seed, TILE_EXTRA_TYPE_SEED)
    uint8 fruitCount = 3;
};  

TILE_EXTRA(TileExtra_Component, TILE_EXTRA_TYPE_COMPONENT)
    uint8 randValue = 0;
};

TILE_EXTRA_GROWTH(TileExtra_Provider, TILE_EXTRA_TYPE_PROVIDER)
    uint32 otherData = 0;
};

TILE_EXTRA(TileExtra_Achievement, TILE_EXTRA_TYPE_ACHIEVEMENT)
    int32 ownerID = -1;
    uint8 achievementID = 127;
};

TILE_EXTRA(TileExtra_HeartMonitor, TILE_EXTRA_TYPE_HEART_MONITOR)
    int32 ownerID = -1;
    string playerDisplayName;
};

TILE_EXTRA(TileExtra_OuijaBoard, TILE_EXTRA_TYPE_OUIJA_BOARD)
    int32 playerCount = 0;
    string ouijaType; // kinda weird 0 for normal 1 for boss
    string command;
    std::vector<int32> items;
};

TILE_EXTRA(TileExtra_FieldNode, TILE_EXTRA_TYPE_FIELD_NODE)
    uint32 expireTime = 0;
    std::vector<int32> nodes;
};

TILE_EXTRA(TileExtra_BattleCage, TILE_EXTRA_TYPE_BATTLE_CAGE)
    string cageName;
    int32 basePet = 0;
    int32 secondPet = 0;
    int32 thirdPet = 0;
};