#include "Utils/StringUtils.h"
#include "RenderWorld.h"
#include "../World/WorldManager.h"
#include "../Player/Dialog/RenderWorldDialog.h"

const CommandInfo& RenderWorld::GetInfo()
{
    static CommandInfo info =
    {
        "/renderworld",
        "View world as image",
        ROLE_PERM_NONE,
        {
            CompileTimeHashString("renderworld")
        }
    };

    return info;
}

void RenderWorld::Execute(GamePlayer* pPlayer, std::vector<string>& args)
{
    if(!pPlayer || args.empty() || !CheckPerm(pPlayer)) {
        return;
    }

    if(pPlayer->GetCurrentWorld() == 0) {
        return;
    }

    if(pPlayer->HasState(PLAYER_STATE_RENDERING_WORLD)) {
        pPlayer->SendOnConsoleMessage("`4OOPS! `oYou already requested for world rendering, you should wait!");
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByID(pPlayer->GetCurrentWorld());
    if(!pWorld) {
        return;
    }

    TileInfo* pLockTile = pWorld->GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);
    if(!pLockTile) {
        pPlayer->SendOnTalkBubble("``Sorry, only `5World Locked`` worlds that you own can be rendered.", false);
        return;
    }

    TileExtra_Lock* pTileExtra = pLockTile->GetExtra<TileExtra_Lock>();
    if(pTileExtra->ownerID != pPlayer->GetUserID()) {
        pPlayer->SendOnTalkBubble("`oSorry, only the world owner can render it.", false);
        return;
    }

    pPlayer->SetState(PLAYER_STATE_RENDERING_WORLD);
    RenderWorldDialog::Request(pPlayer);
}
