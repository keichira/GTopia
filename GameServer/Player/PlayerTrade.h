#pragma once

#include "Precompiled.h"
#include "Utils/Timer.h"

struct TradeItemInfo
{
    int32 itemID = 0;
    int32 count = 0;
};

class InventoryItemInfo;
class GamePlayer;
class ItemInfo;

class PlayerTrade
{
public:
    PlayerTrade(GamePlayer* pPlayer);
    ~PlayerTrade();

public:
    void OnTradeItem(int32 itemID, int32 count, bool askQuantity);
    void OnAddItem(ItemInfo* pItem, InventoryItemInfo* pInvItem, int32 count);
    void OnRemoveItem(int32 itemID);
    bool RemoveFromItems(int32 itemID);
    void SendTradeStatus(bool dealChanged, bool resetAccept, bool forceNotify);
    void SetDelayToAcceptButton(int32 delayMS = 2000);

    void OnEndTrade(GamePlayer* pPlayer);
    void EndTrade();
    void KillTrade();

    string GetStatusText(bool resetLocks);
    string CheckTradingItemsAndGetError();

    void SetLock(bool locked) { m_locked = locked; }
    void SetAccept(bool accepted);

    bool IsBothAcceptedTrade();
    bool IsEnoughTimeElapsedSinceLastAction();
    bool IsTrading() { return (m_pPartner != nullptr); };
    bool IsAccepted() const { return m_accepted; }
    bool HasEnoughSpaceForItems();

    uint32 GetTradingItemsCount() { return m_items.size(); }
    GamePlayer* GetPartner() const { return m_pPartner; }

private:
    GamePlayer* m_pPartner;
    GamePlayer* m_pPlayer;

    bool m_locked;
    bool m_accepted;
    Timer m_lastActionTimer;
    std::vector<TradeItemInfo> m_items;
};