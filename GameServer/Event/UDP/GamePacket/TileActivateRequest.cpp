#include "TileActivateRequest.h"
#include "Utils/GrowUtils.h"
#include "Math/Math.h"

void TileActivateRequest::Execute(GamePlayer* pPlayer, World* pWorld, GameUpdatePacket* pPacket)
{
    if(!pPlayer || !pWorld || !pPacket)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(pPacket->tileX, pPacket->tileY);
    if(!pTile)
        return;

    if(pPlayer->GetDistToTileInTiles(pTile) > 5)
        return;

    
}