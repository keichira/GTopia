#include "PetBattleManager.h"
#include "World.h"
#include "Math/Math.h"

PetBattle::PetBattle(World* pWorld)
{
    m_pWorld = pWorld;
}

PetBattle::~PetBattle()
{
}

bool PetBattle::Init(GamePlayer *pPlayer_1, GamePlayer *pPlayer_2, int32 p1_x, int32 p1_y, int32 p2_x, int32 p2_y)
{
    if(!m_pWorld)
        return false;

    Vector2Float p1_pos;
    Vector2Float p2_pos;

    if(!pPlayer_1)
    {
        TileInfo* pTile = m_pWorld->GetTileManager()->GetTile(p1_x, p1_y);
        if(!pTile)
            return false;

        if(!pTile->HasExtra())
            return false;

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
        if(!pItem)
            return false;

        if(pItem->type != ITEM_TYPE_PET_TRAINER)
            return false;

        p1_pos = pTile->GetWorldPosCenter();
    }
    else
    {
        p1_pos = pPlayer_1->GetWorldPosCenter();
    }

    if(!pPlayer_2)
    {
        TileInfo* pTile = m_pWorld->GetTileManager()->GetTile(p2_x, p2_y);
        if(!pTile)
            return false;

        if(!pTile->HasExtra())
            return false;

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
        if(!pItem)
            return false;

        if(pItem->type != ITEM_TYPE_PET_TRAINER)
            return false;

        p2_pos = pTile->GetWorldPosCenter();
    }
    else
    {
        p2_pos = pPlayer_2->GetWorldPosCenter();
    }

    float deltaX = Abs(p1_pos.x - p2_pos.x);
    float deltaY = Abs(p1_pos.y - p1_pos.y);

    if(deltaX > 32.0f * 5)
    {
        if(pPlayer_1)
        {
            pPlayer_1->SendOnTalkBubble("`4You're too far apart to battle!``", false);
        }

        if(pPlayer_2)
        {
            pPlayer_2->SendOnTalkBubble("`4You're too far apart to battle!``", false);
        }

        return false;
    }

    if(deltaX < 32.0f * 2)
    {
        if(pPlayer_1)
        {
            pPlayer_1->SendOnTalkBubble("`4You're too close to battle!``", false);
        }

        if(pPlayer_2)
        {
            pPlayer_2->SendOnTalkBubble("`4You're too close to battle!``", false);
        }

        return false;
    }

    if(deltaY > 45.0f)
    {
        if(pPlayer_1)
        {
            pPlayer_1->SendOnTalkBubble("`4Battle on level ground!``", false);
        }

        if(pPlayer_2)
        {
            pPlayer_2->SendOnTalkBubble("`4Battle on level ground!``", false);
        }

        return false;
    }

    if(p1_pos.x > p2_pos.x)
    {
        std::swap(pPlayer_1, pPlayer_2);
        std::swap(p1_x, p2_x);
        std::swap(p1_y, p2_y);
    }

    if((pPlayer_1 && !pPlayer_1->GetProgressData().IsBattleLeashFull()) || (pPlayer_2 && !pPlayer_2->GetProgressData().IsBattleLeashFull()))
        return false;

    if(pPlayer_1)
    {
        pPlayer_1->SendOnSetFreezeState(PLAYER_FREEZE_STATE_FROZEN, 0);
    }

    if(pPlayer_2)
    {
        pPlayer_2->SendOnSetFreezeState(PLAYER_FREEZE_STATE_FROZEN, 0);
    }

    for(int8 i = 0; i < 2; ++i)
    {
        PetBattleClientInfo& player = m_players[i];

        player.pPlayer = (i == 0) ? pPlayer_1 : pPlayer_2;
        m_pWorld->SendOnActionToAll(player.pPlayer, "/yes");
    }

    m_lastUpdate.Reset();
}