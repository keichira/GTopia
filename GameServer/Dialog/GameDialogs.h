#pragma once

#include "Packet/PacketUtils.h"
#include "Precompiled.h"
#include "Utils/DialogPagination.h"

class ItemInfo;
class World;
class InventoryItemInfo;
class GamePlayer;
class TileInfo;

class AchievementBlockDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class BattleCageDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class BulletinBlockDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void HandleDeleteEntry(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);

private:
    static void RequestDeleteEntry(GamePlayer* pPlayer, TileInfo* pTile, int32 index);
    static void SendBulletinDialog(GamePlayer* pPlayer, World* pWorld, TileInfo* pTile, ItemInfo* pItem);
};

class BurglarDialog
{
public:
    static void RequestPunch(GamePlayer* pPlayer, TileInfo* pTile);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class CrystalBlockDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
};

class DisplayBlockDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile);
    static void RequestPutItem(GamePlayer* pPlayer, TileInfo* pTile, int32 itemID);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class DonationBoxDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void EmptyDonationBox(GamePlayer* pPlayer, TileInfo* pTile, bool allowDrop,
                                 const std::vector<bool>& selectedGifts = {});
    static void RequestDonatingItem(GamePlayer* pPlayer, TileInfo* pTile, int32 itemID);
    static void HandleGiveItem(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class DoorDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void RequestPasswordDoor(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void HandlePasswordReply(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class DressupDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile);
    static bool RequestPunch(GamePlayer* pPlayer, TileInfo* pTile);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void HandleAsk(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class DropItemDialog
{
public:
    static void Request(GamePlayer* pPlayer, InventoryItemInfo* pInvItem);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class GatewayDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class ItemFinderDialog : public DialogPagination
{
public:
    explicit ItemFinderDialog(std::vector<int16>&& itemIds)
        : DialogPagination(30), m_fileteredItemIds(std::move(itemIds))
    {
    }

    std::string_view GetDialogName() const override { return "item_finder"; }
    uint32 GetTotalCount() const override { return m_fileteredItemIds.size(); }

    void Render(GamePlayer* pPlayer) override;
    void OnSelectElement(GamePlayer* pPlayer, uint32 absoluteIndex) override;

private:
    std::vector<int16> m_fileteredItemIds;
};

class LockDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile);
    static void HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class MailboxBlockDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class MannequinDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile);
    static void RequestPutItem(GamePlayer* pPlayer, TileInfo* pTile, int32 itemID, bool fromDialog);
    static bool RequestRemoveItem(GamePlayer* pPlayer, TileInfo* pTile);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};
class OuijaBoardDialog
{
private:
    enum class eOuijaItemInfoType
    {
        MAIN,
        NAME,
        AMOUNT,
        RARITY,
        SEED
    };

public:
    static void RequestMain(GamePlayer* pPlayer, TileInfo* pTile);
    static void RequestItemInfo(GamePlayer* pPlayer, TileInfo* pTile, int32 itemIndex, eOuijaItemInfoType type);
    static void RequestCommand(GamePlayer* pPlayer, TileInfo* pTile);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class PopupDialog
{
public:
    static void RequestOther(GamePlayer* pPlayer, GamePlayer* pTarget);
    static void RequestSelf(GamePlayer* pPlayer);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void HandleTitleEdit(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void HandleAcceptAccess(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void HandleBillboardEdit(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);

private:
    static void RequestTitleEdit(GamePlayer* pPlayer);
    static void RequestBillboardEdit(GamePlayer* pPlayer);
};

class RegisterDialog
{
public:
    static void Request(GamePlayer* pPlayer, const string& namePlaceholder = "", const string& passPlaceholder = "",
                        const string& passVerifPlaceholder = "", const string& errorMsg = "");
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void Success(GamePlayer* pPlayer, const string& growID, const string& pass);
};

class RenderWorldDialog
{
public:
    static void Request(GamePlayer* pPlayer);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void OnRendered(GamePlayer* pPlayer, const string& worldName);
};

class SignDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class SpotlightDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class SuckerBlockDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile);
    static void RequestAddItem(GamePlayer* pPlayer, TileInfo* pTile);
    static void RequestRetrieveItem(GamePlayer* pPlayer, TileInfo* pTile);

    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void HandleAddItem(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void HandleRetrieveItem(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class TradeDialog
{
public:
    static void Request(GamePlayer* pPlayer, InventoryItemInfo* pInvItem);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class TrashDialog
{
public:
    static void Request(GamePlayer* pPlayer, int16 itemID);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void HandleUntradeable(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class VendingMachineDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class WeatherSpecialDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};

class XenoniteDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};