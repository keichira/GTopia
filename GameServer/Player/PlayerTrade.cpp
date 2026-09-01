#include "PlayerTrade.h"
#include "../Dialog/GameDialogs.h"
#include "GamePlayer.h"
#include "Item/ItemInfoManager.h"

PlayerTrade::PlayerTrade(GamePlayer* pPlayer)
    : m_pPlayer(pPlayer), m_pPartner(nullptr), m_accepted(false), m_locked(false)
{
    m_lastActionTimer.Set(0);
}

PlayerTrade::~PlayerTrade() {}

void PlayerTrade::OnTradeItem(int32 itemID, int32 count, bool askQuantity)
{
    if (!m_pPlayer)
        return;

    InventoryItemInfo* pInvItem = m_pPlayer->GetInventory().GetItemByID(itemID);
    if (!pInvItem)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    if (pItem->HasFlag(ITEM_FLAG_UNTRADEABLE) || pItem->IsUnlimited())
    {
        m_pPlayer->SendOnTextOverlay("You'd be sorry if you lost that!");
        m_pPlayer->PlaySFX("cant_place_tile.wav");
        return;
    }

    if (pInvItem->count < count)
        return;

    if (pItem->type == ITEM_TYPE_PETFISH)
    {
        askQuantity = false;
        count = pInvItem->count;
    }

    if (itemID == ITEM_ID_WORLD_KEY)
    {
        // todo
        return;
    }

    if (itemID == ITEM_ID_GUILD_KEY)
    {
        // todo
        return;
    }

    if (!askQuantity || count != 1 || pInvItem->count < 2)
    {
        OnAddItem(pItem, pInvItem, count);
    }
    else
    {
        TradeDialog::Request(m_pPlayer, pInvItem);
    }
}

void PlayerTrade::OnAddItem(ItemInfo* pItem, InventoryItemInfo* pInvItem, int32 count)
{
    if (!pItem || !pInvItem || !m_pPlayer)
        return;

    if (!IsTrading())
        return;

    if (m_items.size() >= 4)
    {
        m_pPlayer->SendOnTextOverlay("Can't add another item!");
        return;
    }

    if (!IsEnoughTimeElapsedSinceLastAction())
        return;

    if (pItem->id == ITEM_ID_WORLD_KEY)
    {
        // todo
    }

    if (pItem->id == ITEM_ID_GUILD_KEY)
    {
        // todo
    }

    RemoveFromItems(pItem->id);
    m_items.emplace_back(TradeItemInfo{pItem->id, count});

    if (m_pPartner)
    {
        PlayerTrade& partnerTradeMgr = m_pPartner->GetTradeManager();
        if (partnerTradeMgr.GetPartner() == m_pPlayer)
        {
            string changeStatus =
                "`1TRADE CHANGE: `` " + m_pPartner->GetRawName() + " added `w" + ToString(count) + "`` " + pItem->name;
            m_pPartner->SendOnConsoleMessage(changeStatus);
        }
    }

    SendTradeStatus(true, true, true);
}

void PlayerTrade::OnRemoveItem(int32 itemID)
{
    if (IsTrading() && !IsBothAcceptedTrade() && IsEnoughTimeElapsedSinceLastAction() && RemoveFromItems(itemID))
    {
        SendTradeStatus(true, true, true);
    }
}

bool PlayerTrade::RemoveFromItems(int32 itemID)
{
    for (uint32 i = 0; i < m_items.size(); ++i)
    {
        if (m_items[i].itemID == ITEM_ID_BLANK || m_items[i].itemID != itemID)
            continue;

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(m_items[i].itemID);
        if (m_pPlayer && m_pPartner && pItem)
        {
            PlayerTrade& partnerTradeMgr = m_pPartner->GetTradeManager();
            if (partnerTradeMgr.GetPartner() == m_pPlayer)
            {
                string changeStatus = "`1TRADE CHANGE: `` " + m_pPartner->GetRawName() + " removed `w" +
                                      ToString(m_items[i].count) + "`` " + pItem->name;
                m_pPartner->SendOnConsoleMessage(changeStatus);
            }
        }

        m_items.erase(m_items.begin() + i);
        return true;
    }

    return false;
}

void PlayerTrade::SendTradeStatus(bool dealChanged, bool resetAccept, bool forceNotify)
{
    if (!m_pPlayer)
        return;

    if (dealChanged && m_accepted)
    {
        m_pPlayer->SendOnTextOverlay("The deal has changed");
        m_pPlayer->PlaySFX("tile_removed.wav");
    }

    if (m_pPartner)
    {
        m_pPlayer->SendOnTradeStatus(m_pPartner->GetNetID(), "", m_pPartner->GetRawName() + "'s offer.``",
                                     GetStatusText(false));

        PlayerTrade& partnerTradeMgr = m_pPartner->GetTradeManager();
        if (partnerTradeMgr.GetPartner() == m_pPlayer)
        {
            if (dealChanged)
            {
                if (partnerTradeMgr.IsAccepted() || forceNotify)
                {
                    m_pPartner->SendOnTextOverlay("The deal has changed.");
                    m_pPartner->PlaySFX("tile_removed.wav");
                }

                partnerTradeMgr.SetDelayToAcceptButton();
            }
            else if (resetAccept)
            {
                if (forceNotify)
                {
                    m_pPartner->SendOnTextOverlay("The deal has changed.");
                    m_pPartner->PlaySFX("tile_removed.wav");
                }

                partnerTradeMgr.SetDelayToAcceptButton();
            }

            m_pPartner->SendOnTradeStatus(m_pPartner->GetNetID(), "", m_pPartner->GetRawName() + "'s offer.``",
                                          GetStatusText(false));
        }
    }
}

string PlayerTrade::GetStatusText(bool resetLocks)
{
    string res;

    for (uint32 i = 0; i < m_items.size(); ++i)
    {
        if (m_items[i].itemID == ITEM_ID_BLANK)
            continue;

        res += "slot|" + ToString(i) + "|" + ToString(m_items[i].itemID) + "|" + ToString(m_items[i].count) + "\n";
    }

    res += "accepted|" + ToString(m_accepted ? 1 : 0) + "\n";

    if (resetLocks)
    {
        res += "reset_locks|1\n";
    }

    res += "locked|" + ToString(m_locked ? 1 : 0) + "\n";
    return res;
}

string PlayerTrade::CheckTradingItemsAndGetError()
{
    if (!m_pPlayer)
        return "Something happened bad.";

    for (auto& item : m_items)
    {
        InventoryItemInfo* pInvItem = m_pPlayer->GetInventory().GetItemByID(item.itemID);
        if (!pInvItem || pInvItem->count < item.count)
            return "`4Oops Items missing from trade!``";

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(item.itemID);
        if (!pItem)
            return "`4Oops Items missing from database!``";

        if (item.itemID == ITEM_ID_WORLD_LOCK)
        {
            // todo
        }
        else if (item.itemID == ITEM_ID_GUILD_KEY)
        {
            // todo
        }
        else
        {
            if (pItem->type == ITEM_TYPE_PETFISH && m_pPartner)
            {
                if (m_pPartner->GetInventory().GetCountOfItem(item.itemID) > 0)
                    return "`4Oops - can't trade a fish to someone already holding that fish type!``";
            }

            if (m_pPartner)
            {
                if (m_pPartner->GetInventory().GetCountOfItem(item.itemID) + item.count > pItem->maxCanHold)
                    return m_pPartner->GetRawName() + " is carrying too many " + pItem->name +
                           " and can't fit that many in their backpack.";
            }

            if (m_pPlayer->GetInventory().GetCountOfItem(item.itemID) + item.count > pItem->maxCanHold)
                return m_pPlayer->GetRawName() + " doesn't have enough free backpack slots available.";
        }
    }

    return "";
}

void PlayerTrade::SetDelayToAcceptButton(int32 delayMS)
{
    m_lastActionTimer.Reset(delayMS);

    if (!m_pPlayer || !m_pPartner)
        return;

    m_locked = true;
    m_pPlayer->SendOnTradeStatus(m_pPartner->GetNetID(), "", m_pPartner->GetRawName() + "'s offer.``",
                                 GetStatusText(false));

    m_locked = false;
    m_pPlayer->SendOnTradeStatus(m_pPartner->GetNetID(), "", m_pPartner->GetRawName() + "'s offer.``",
                                 GetStatusText(false));
}

void PlayerTrade::OnEndTrade(GamePlayer* pPlayer)
{
    if (!pPlayer || pPlayer != m_pPartner)
        return;

    m_pPlayer->SendOnTextOverlay(pPlayer->GetRawName() + " has canceled the trade");
    m_pPlayer->SendOnForceTradeEnded();
}

void PlayerTrade::EndTrade()
{
    m_pPartner = nullptr;
    m_accepted = false;
    m_locked = false;
    m_items.clear();
    m_lastActionTimer.Set(0); // todo
}

void PlayerTrade::KillTrade()
{
    if (m_pPartner)
    {
        m_pPartner->GetTradeManager().OnEndTrade(m_pPlayer);
    }

    EndTrade();
}

void PlayerTrade::SetAccept(bool accepted)
{
    if (!m_pPlayer || !m_pPartner || IsBothAcceptedTrade())
        return;

    if (accepted && !m_lastActionTimer.IsPassed())
    {
        SetDelayToAcceptButton();
        return;
    }

    if (m_accepted == accepted)
        return;

    PlayerTrade& partnerTradeMgr = m_pPartner->GetTradeManager();
    if (partnerTradeMgr.GetPartner() == m_pPlayer && accepted)
    {
        if (partnerTradeMgr.IsAccepted())
        {
            string reqError = CheckTradingItemsAndGetError();
            if (!reqError.empty())
            {
                m_pPartner->SendOnTextOverlay(reqError);
                m_pPlayer->SendOnTextOverlay(reqError);
                accepted = false;
            }

            if (accepted)
            {
                string partnerReqError = partnerTradeMgr.CheckTradingItemsAndGetError();
                if (!partnerReqError.empty())
                {
                    m_pPartner->SendOnTextOverlay(partnerReqError);
                    m_pPlayer->SendOnTextOverlay(partnerReqError);
                    accepted = false;
                }
            }

            if (accepted && !HasEnoughSpaceForItems())
            {
                string bpSpace = m_pPlayer->GetRawName() + " needs more backpack room first!";
                m_pPartner->SendOnTextOverlay(bpSpace);
                m_pPlayer->SendOnTextOverlay(bpSpace);
                accepted = false;
            }

            if (accepted)
            {
                if (!partnerTradeMgr.HasEnoughSpaceForItems())
                {
                    string bpSpace = m_pPartner->GetRawName() + " needs more backpack room first!";
                    m_pPartner->SendOnTextOverlay(bpSpace);
                    m_pPlayer->SendOnTextOverlay(bpSpace);
                    accepted = false;
                }
            }

            //*
        }

        m_accepted = accepted;
        if (accepted)
        {
            SendTradeStatus(false, false, false);
            return;
        }

        SendTradeStatus(false, true, false);
    }
}

bool PlayerTrade::IsBothAcceptedTrade()
{
    if (!m_pPartner || !m_accepted)
        return false;

    return m_pPartner->GetTradeManager().IsAccepted();
}

bool PlayerTrade::IsEnoughTimeElapsedSinceLastAction()
{
    if (m_lastActionTimer.IsPassed())
    {
        if (m_pPlayer)
        {
            m_pPlayer->SendOnTextOverlay("Slow down!  Please wait a second between adding and removing item");
        }

        return false;
    }
    else
    {
        m_lastActionTimer.Set(500);
        return true;
    }

    return false;
}

bool PlayerTrade::HasEnoughSpaceForItems()
{
    if (!m_pPlayer)
        return false;

    return (m_pPlayer->GetInventory().GetInventorySize() -
            (m_pPlayer->GetInventory().GetItemsSize() + m_items.size())) >= 0;
}
