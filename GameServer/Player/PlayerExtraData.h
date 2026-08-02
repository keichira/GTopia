#pragma once

#include "Memory/MemoryBuffer.h"
#include "Precompiled.h"
#include "Utils/Variant.h"

class GamePlayer;

enum ePlayerExtraKey : uint16
{
    PLAYER_EXTRA_BILLBOARD_ITEM_ID,
    PLAYER_EXTRA_BILLBOARD_SHOW,
    PLAYER_EXTRA_BILLBOARD_PRICE,
    PLAYER_EXTRA_BILLBOARD_IS_LOCK_PER,
    PLAYER_EXTRA_BILLBOARD_IS_BUY
};

class PlayerExtraData
{
public:
    PlayerExtraData(GamePlayer* pPlayer);
    ~PlayerExtraData();

public:
    bool Set(ePlayerExtraKey key, const Variant& data);

    template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, Variant>>>
    bool Set(ePlayerExtraKey key, T&& value)
    {
        return Set(key, Variant(std::forward<T>(value)));
    }

    Variant& Get(ePlayerExtraKey key);

    void Serialize(MemoryBuffer& memBuffer, bool write);
    uint32 GetMemEstimate();

private:
    eVariantTypes GetExpectedTypeForKey(ePlayerExtraKey key);

private:
    GamePlayer* m_pPlayer;
    VariantMap<uint16> m_data;
};