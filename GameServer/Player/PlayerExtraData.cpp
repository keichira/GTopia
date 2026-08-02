#include "PlayerExtraData.h"
#include "GamePlayer.h"
#include "IO/Log.h"

PlayerExtraData::PlayerExtraData(GamePlayer* pPlayer) : m_pPlayer(pPlayer) {}

PlayerExtraData::~PlayerExtraData() {}

bool PlayerExtraData::Set(ePlayerExtraKey key, const Variant& data)
{
    if (GetExpectedTypeForKey(key) != data.GetType())
    {
        LOGGER_LOG_ERROR("[PlayerExtraData] Non valid type for key: %d, income: %d, expected: %d, player: %d", key,
                         (int32)data.GetType(), (int32)GetExpectedTypeForKey(key),
                         (m_pPlayer ? m_pPlayer->GetUserID() : -1));
        return false;
    }

    m_data[key] = data;
    return true;
}

Variant& PlayerExtraData::Get(ePlayerExtraKey key)
{
    auto it = m_data.find(key);
    if (it != m_data.end())
        return it->second;

    m_data.insert_or_assign(key, Variant());
    return m_data[key];
}

void PlayerExtraData::Serialize(MemoryBuffer& memBuffer, bool write)
{
    uint16 dataCount = m_data.size();
    memBuffer.ReadWrite(dataCount, write);

    if (write)
    {
        memBuffer.Write(dataCount);

        for (auto& [key, var] : m_data)
        {
            if (var.GetType() == VARIANT_TYPE_NONE)
                continue;

            memBuffer.Write(key);
            memBuffer.Write((uint8)var.GetType());

            switch (var.GetType())
            {
                case VARIANT_TYPE_STRING:
                    memBuffer.WriteStringRaw(var.GetString());
                    break;
                case VARIANT_TYPE_FLOAT:
                    memBuffer.Write(var.GetFloat());
                    break;
                case VARIANT_TYPE_UINT:
                    memBuffer.Write(var.GetUINT());
                    break;
                case VARIANT_TYPE_BOOL:
                    memBuffer.Write(var.GetBool());
                    break;
                case VARIANT_TYPE_INT:
                    memBuffer.Write(var.GetINT());
                    break;
                case VARIANT_TYPE_VECTOR2INT:
                    memBuffer.Write(var.GetVector2Int());
                    break;
                case VARIANT_TYPE_VECTOR2FLOAT:
                    memBuffer.Write(var.GetVector2Float());
                    break;
                case VARIANT_TYPE_VECTOR3INT:
                    memBuffer.Write(var.GetVector3Int());
                    break;
                case VARIANT_TYPE_VECTOR3FLOAT:
                    memBuffer.Write(var.GetVector3Float());
                    break;
            }
        }
    }
    else
    {
        m_data.clear();

        for (uint16 i = 0; i < dataCount; ++i)
        {
            uint16 key = 0;
            memBuffer.Read(key);

            uint8 type = 0;
            memBuffer.Read(type);

            Variant var;
            switch (type)
            {
                case VARIANT_TYPE_STRING:
                {
                    string str = "";
                    memBuffer.ReadStringRaw(str);
                    var = str;
                    break;
                }
                case VARIANT_TYPE_FLOAT:
                {
                    float val = 0;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                case VARIANT_TYPE_UINT:
                {
                    uint32 val = 0;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                case VARIANT_TYPE_BOOL:
                {
                    bool val = false;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                case VARIANT_TYPE_INT:
                {
                    int32 val = 0;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                case VARIANT_TYPE_VECTOR2INT:
                {
                    Vector2Int val;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                case VARIANT_TYPE_VECTOR2FLOAT:
                {
                    Vector2Float val;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                case VARIANT_TYPE_VECTOR3INT:
                {
                    Vector3Int val;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
                case VARIANT_TYPE_VECTOR3FLOAT:
                {
                    Vector3Float val;
                    memBuffer.Read(val);
                    var = val;
                    break;
                }
            }

            m_data[key] = var;
        }
    }
}

uint32 PlayerExtraData::GetMemEstimate()
{
    uint32 memEstimate = 2;
    for (auto& [_, data] : m_data)
    {
        if (data.GetType() == VARIANT_TYPE_NONE)
            continue;

        memEstimate += (2 + data.GetSize());
        if (data.GetType() == VARIANT_TYPE_STRING)
            memEstimate += 2;
    }

    return memEstimate;
}

eVariantTypes PlayerExtraData::GetExpectedTypeForKey(ePlayerExtraKey key)
{
    switch (key)
    {
        case PLAYER_EXTRA_BILLBOARD_ITEM_ID:
            return VARIANT_TYPE_INT;
        case PLAYER_EXTRA_BILLBOARD_SHOW:
            return VARIANT_TYPE_BOOL;
        case PLAYER_EXTRA_BILLBOARD_PRICE:
            return VARIANT_TYPE_INT;
        case PLAYER_EXTRA_BILLBOARD_IS_LOCK_PER:
            return VARIANT_TYPE_BOOL;
        case PLAYER_EXTRA_BILLBOARD_IS_BUY:
            return VARIANT_TYPE_BOOL;

        default:
            return VARIANT_TYPE_NONE;
    }
}
