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
    TCP_PACKET_ADMIN_COMMAND,

    TCP_RENDER_REQUEST = 1000,
    TCP_RENDER_RESULT = 10001,

    TCP_COMMAND_SETROLE = 1100
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
    NET_GAME_PACKET_SEND_PLAYER_TRIBUTE_DATA,
    NET_GAME_PACKET_ON_STEP_ONILE_MOD
};

enum eGamePacketFlags 
{
    GAME_PACKET_FLAG_BROADCAST = 1 << 0,
    GAME_PACKET_FLAG_NO_LERP = 1 << 2,
    GAME_PACKET_FLAG_EXTENDED_DATA = 1 << 3,
    GAME_PACKET_FLAG_FACING_LEFT = 1 << 4,
    GAME_PACKET_FLAG_ON_GROUND = 1 << 5,
    GAME_PACKET_FLAG_ON_BURN = 1 << 6,
    GAME_PACKET_FLAG_ON_JUMP = 1 << 7,
    GAME_PACKET_FLAG_ON_KILL = 1 << 8,
    GAME_PACKET_FLAG_ON_PUNCH = 1 << 9,
    GAME_PACKET_FLAG_ON_CREATE = 1 << 10,
    GAME_PACKET_FLAG_SEND_ASAP = 1 << 11,
    GAME_PACKET_FLAG_ON_GOT_PUNCH = 1 << 12,
    GAME_PACKET_FLAG_ON_RESPAWN = 1 << 13,
    GAME_PACKET_FLAG_ON_GOT_OBJECT = 1 << 14,
    GAME_PACKET_FLAG_ON_TRAMPOLINE = 1 << 15,
    GAME_PACKET_FLAG_ON_PRICK = 1 << 16,
    GAME_PACKET_FLAG_ON_ICE = 1 << 17,
    GAME_PACKET_FLAG_HAND_ITEM_ACTIVE = 1 << 18,
    GAME_PACKET_FLAG_BACK_ITEM_ACTIVE = 1 << 19,
    GAME_PACKET_FLAG_UNDERWATER = 1 << 20,
    GAME_PACKET_FLAG_WALLHANG = 1 << 21,
    GAME_PACKET_FLAG_CHARGING_PUNCH = 1 << 22,
    GAME_PACKET_FLAG_CHARGING_PUNCH_END = 1 << 23,
    GAME_PACKET_FLAG_CHARGING_PUNCH_LEVEL_UP = 1 << 24,
    GAME_PACKET_FLAG_PLAY_HAY_CART_VISUALS = 1 << 25,
    GAME_PACKET_FLAG_DOUBLE_JUMP_ACTIVE = 1 << 26,
    GAME_PACKET_FLAG_EXTRA_TILE_INFO = 1 << 27,
    GAME_PACKET_FLAG_ON_ACID = 1 << 28,
    GAME_PACKET_FLAG_TELEPORT = 1 << 29,
    GAME_PACKET_FLAG_ON_USE_ABILITY = 1 << 30
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

    uint32 flags = 0;

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
        int32 actualItemDataSize;
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