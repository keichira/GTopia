#pragma once

#include "../Precompiled.h"
#include "../Utils/Variant.h"
#include "../IO/Log.h"

enum eTCPPacketType
{
    TCP_PACKET_HELLO,
    TCP_PACKET_AUTH,
    TCP_PACKET_PLAYER_CHECK_SESSION,
    TCP_PACKET_PLAYER_END_SESSION,
    TCP_PACKET_WORLD_INIT,
    TCP_PACKET_RENDER_WORLD,
    TCP_PACKET_HEARTBEAT,
    TCP_PACKET_WORLD_SEND_PLAYER,
    TCP_PACKET_KILL_SERVER,

    TCP_RENDER_REQUEST = 1000,
    TCP_RENDER_RESULT = 10001
};

enum eTCPPacketResult
{
    TCP_RESULT_FAIL,
    TCP_RESULT_OK
};

enum eMessagePacketType 
{
    NET_MESSAGE_UNKNOWN,
    NET_MESSAGE_SERVER_HELLO,
    NET_MESSAGE_GENERIC_TEXT,
    NET_MESSAGE_GAME_MESSAGE,
    NET_MESSAGE_GAME_PACKET,
    NET_MESSAGE_ERROR,
    NET_MESSAGE_TRACK,
    NET_MESSAGE_CLIENT_LOG_REQUEST,
    NET_MESSAGE_CLIENT_LOG_RESPONSE
};

enum eGamePacketType 
{
    NET_GAME_PACKET_STATE,
    NET_GAME_PACKET_CALL_FUNCTION,
    NET_GAME_PACKET_UPDATE_STATUS,
    NET_GAME_PACKET_TILE_CHANGE_REQUEST,
    NET_GAME_PACKET_SEND_MAP_DATA,
    NET_GAME_PACKET_SEND_TILE_UPDATE_DATA,
    NET_GAME_PACKET_SEND_TILE_UPDATE_DATA_MULTIPLE,
    NET_GAME_PACKET_TILE_ACTIVATE_REQUEST,
    NET_GAME_PACKET_TILE_APPLY_DAMAGE,
    NET_GAME_PACKET_SEND_INVENTORY_STATE,
    NET_GAME_PACKET_ITEM_ACTIVATE_REQUEST,
    NET_GAME_PACKET_ITEM_ACTIVATE_OBJECT_REQUEST,
    NET_GAME_PACKET_SEND_TILE_TREE_STATE,
    NET_GAME_PACKET_MODIFY_ITEM_INVENTORY,
    NET_GAME_PACKET_ITEM_CHANGE_OBJECT,
    NET_GAME_PACKET_SEND_LOCK,
    NET_GAME_PACKET_SEND_ITEM_DATABASE_DATA,
    NET_GAME_PACKET_SEND_PARTICLE_EFFECT,
    NET_GAME_PACKET_SET_ICON_STATE,
    NET_GAME_PACKET_ITEM_EFFECT,
    NET_GAME_PACKET_SET_CHARACTER_STATE,
    NET_GAME_PACKET_PING_REPLY,
    NET_GAME_PACKET_PING_REQUEST,
    NET_GAME_PACKET_GOT_PUNCHED,
    NET_GAME_PACKET_APP_CHECK_RESPONSE,
    NET_GAME_PACKET_APP_INTEGRITY_FAIL,
    NET_GAME_PACKET_DISCONNECT,
    NET_GAME_PACKET_BATTLE_JOIN,
    NET_GAME_PACKET_BATTLE_EVENT,
    NET_GAME_PACKET_USE_DOOR,
    NET_GAME_PACKET_SEND_PARENTAL,
    NET_GAME_PACKET_GONE_FISHIN,
    NET_GAME_PACKET_STEAM,
    NET_GAME_PACKET_PET_BATTLE,
    NET_GAME_PACKET_NPC,
    NET_GAME_PACKET_SPECIAL,
    NET_GAME_PACKET_SEND_PARTICLE_EFFECT_V2,
    NET_GAME_PACKET_ACTIVE_ARROWO_ITEM,
    NET_GAME_PACKET_SELECTILE_INDEX,
    NET_GAME_PACKET_SEND_PLAYERRIBUTE_DATA,
    NET_GAME_PACKET_ON_STEP_ONILE_MOD
};

enum eGamePacketFlags 
{
    NET_GAME_PACKET_FLAGS_NONE = 0,
    NET_GAME_PACKET_FLAGS_FLYING = 1 << 1,
    NET_GAME_PACKET_FLAGS_UPDATE = 1 << 2,
    NET_GAME_PACKET_FLAGS_EXTENDED = 1 << 3,
    NET_GAME_PACKET_FLAGS_FACINGLEFT = 1 << 4,
    NET_GAME_PACKET_FLAGS_ONGROUND = 1 << 5,
    NET_GAME_PACKET_FLAGS_LAVA = 1 << 6,
    NET_GAME_PACKET_FLAGS_JUMP = 1 << 7,
    NET_GAME_PACKET_FLAGS_DEATH = 1 << 8,
    NET_GAME_PACKET_FLAGS_PUNCH = 1 << 9,
    NET_GAME_PACKET_FLAGS_PLACE = 1 << 10,
    NET_GAME_PACKET_FLAGS_TILEACTION = 1 << 11,
    NET_GAME_PACKET_FLAGS_KNOCKBACK = 1 << 12,
    NET_GAME_PACKET_FLAGS_RESPAWN = 1 << 13,
    NET_GAME_PACKET_FLAGS_PICKUP = 1 << 14,
    NET_GAME_PACKET_FLAGS_TRAMPOLINE = 1 << 15,
    NET_GAME_PACKET_FLAGS_CACTUS = 1 << 16,
    NET_GAME_PACKET_FLAGS_SLIDING = 1 << 17,
    NET_GAME_PACKET_FLAGS_JUMPPEAK = 1 << 18,
    NET_GAME_PACKET_FLAGS_FALLING_SLOWLY = 1 << 19,
    NET_GAME_PACKET_FLAGS_SWIM = 1 << 20,
    NET_GAME_PACKET_FLAGS_WALLHANG = 1 << 21,
    NET_GAME_PACKET_FLAGS_RAYMAN_START = 1 << 22,
    NET_GAME_PACKET_FLAGS_RAYMAN_END = 1 << 23,
    NET_GAME_PACKET_FLAGS_RAYMAN_LOAD = 1 << 24,
    NET_GAME_PACKET_FLAGS_FORCE_RING = 1 << 25,
    NET_GAME_PACKET_FLAGS_CACTUS_RAPE = 1 << 26,
    // :)
    NET_GAME_PACKET_FLAGS_ACID = 1 << 28
};

#pragma pack(push, 1)
struct GameUpdatePacket
{
    GameUpdatePacket() { Clear(); }
    void Clear() { memset(this, 0, sizeof(GameUpdatePacket)); }

    uint8 type = NET_GAME_PACKET_STATE;

    union
    {
        uint8 field0 = 0;
        uint8 characterPunchType;
        uint8 worldObjectFlags;
    };

    union
    {
        uint8 field1 = 0;
        uint8 jumpCount;
        uint8 buildRange;
        uint8 removedItemCount;
    };

    union
    {
        uint8 field2 = 0;
        uint8 animType;
        uint8 punchRange;
        uint8 addedItemCount;
    };

    union
    {
        int32 field3 = 0;
        int32 netID;
        int32 userID;
        int32 worldObjectType;
    };

    union
    {
        int32 field4 = 0;
        int32 tileCount;
    };

    int32 flags = 0;

    union
    {
        float field5 = 0;
        float characterWaterSpeed;
        float worldObjectCount;
    };

    union
    {
        int32 field6 = 0;
        int32 delay;
        int32 itemID;
        int32 tileDamage;
        int32 characterFlags;
        int32 worldObjectID;
    };

    union
    {
        float field7 = 0;
        float characterSpeed;
        float posX;
        float speedX;
    };

    union
    {
        float field8 = 0;
        float characterPunchPower;
        float posY;
        float speedY;
    };

    union
    {
        float field9 = 0;
        float characterAccel;
        float particleEffectSize;
    };

    union
    {
        float field10 = 0;
        float characterGravity;
        float particleEffectType;
    };

    union
    {
        float field11 = 0;
        float characterFireDamage;
    };

    union
    {
        uint32 field12 = 0;
        uint32 tileX;
    };

    union
    {
        uint32 field13 = 0;
        uint32 tileY;
    };

    uint32 extraDataSize = 0;

    bool HasFlag(eGamePacketFlags flag) { return flags & flag; }
    void SetFlag(eGamePacketFlags flag) { flags |= flag; }

    void Print() 
    {
        LOGGER_LOG_DEBUG(
            "field0=%u, field1=%u, field2=%u, field3=%d, field4=%d, flags=%d, "
            "field5=%f, field6=%d, field7=%f, field8=%f, field9=%f, field10=%f, "
            "field11=%f, field12=%d, field13=%d, extraDataSize=%u\n",
            field0, field1, field2, field3, field4, flags,
            field5, field6, field7, field8, field9, field10,
            field11, field12, field13, extraDataSize
        );
    }
};
#pragma pack(pop)