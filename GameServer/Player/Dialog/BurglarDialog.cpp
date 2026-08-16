#include "BurglarDialog.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include "Utils/DialogBuilder.h"

void BurglarDialog::RequestPunch(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;
}

void BurglarDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet) {}
