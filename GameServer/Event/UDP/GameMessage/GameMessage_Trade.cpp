#include "GameMessage_Trade.h"

void GameMessage_ModTrade(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if(!pPlayer)
        return;

    if(auto pItemID = packet.Find("itemID"_hash))
    {
        int32 itemID = 0;
        if(pItemID->GetInt(itemID) != TO_INT_SUCCESS)
        {
            pPlayer->GetTradeManager().OnTradeItem(itemID, 1, false);
        }
    }
}

void Gamemessage_RemTrade(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if(!pPlayer)
        return;

    if(auto pItemID = packet.Find("itemID"_hash))
    {
        int32 itemID = 0;
        if(pItemID->GetInt(itemID) != TO_INT_SUCCESS)
        {
            pPlayer->GetTradeManager().OnRemoveItem(itemID);
        }
    }
}

void GameMessage_TradeAccept(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if(!pPlayer)
        return;

    if(auto pItemID = packet.Find("status"_hash))
    {
        bool status;
        if(pItemID->GetBool(status) != TO_INT_SUCCESS)
        {
            pPlayer->GetTradeManager().SetAccept(status);
        }
    }
}

void GameMessage_TradeCancel(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if(!pPlayer)
        return;

    pPlayer->GetTradeManager().KillTrade();
}

void GameMessage_TradeLock(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if(!pPlayer)
        return;

    if(auto pItemID = packet.Find("status"_hash))
    {
        bool status;
        if(pItemID->GetBool(status) != TO_INT_SUCCESS)
        {
            pPlayer->GetTradeManager().SetLock(status);
        }
    }
}

void GameMessage_TradeStarted(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if(!pPlayer)
        return;

    if(auto pNetID = packet.Find("netID"_hash))
    {
        int32 netID = 0;
        if(pNetID->GetInt(netID) != TO_INT_SUCCESS)
        {
            
        }
    }
}