#include "GameMessage_Player.h"
#include "../../../Context.h"
#include "../../../Server/GameServer.h"
#include "../../../World/WorldManager.h"
#include "Database/Table/PlayerDBTable.h"

void GameMessage_EnterGame(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer || !pPlayer->HasState(PLAYER_STATE_ENTERING_GAME))
        return;

    QueryRequest req = PlayerDB::GetData(pPlayer->GetUserID(), pPlayer->GetNetID());
    req.callback = &GamePlayer::OnEnterGameCB;
    DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
}

void GameMessage_GrowID(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer || pPlayer->HasGrowID())
        return;

    pPlayer->CheckLimitsForAccountCreation(false);
}

void GameMessage_Quit(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    pPlayer->LogOff(true, true, true);
}

void GameMessage_QuitToExit(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    if (pPlayer->GetCurrentWorld() == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    pWorld->PlayerLeaveWorld(pPlayer, true);
}

void GameMessage_RefreshTributeData(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    /*PlayerTributeClientData* clientData =
    GetPlayerTributeManager()->GetClientData(pPlayer->GetLoginDetail().protocol); if(!clientData->pData)
    {
        LOGGER_LOG_WARN("Not sending player tribute data because its NULL");
        return;
    }

    GameUpdatePacket gamePacket;
    gamePacket.type = NET_GAME_PACKET_SEND_PLAYER_TRIBUTE_DATA;
    gamePacket.field_4 = -1;
    gamePacket.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;
    gamePacket.extraDataSize = clientData->size;*/
}

void GameMessage_JoinRequest(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (pPlayer->GetLastJoinRequestTime().GetElapsedTime() <= 800)
        return;

    pPlayer->GetLastJoinRequestTime().Reset();

    auto pName = packet.Find("name"_hash);
    if (!pName || pName->valueSize == 0)
    {
        pPlayer->SendOnFailedToEnterWorld();
        return;
    }

    pPlayer->SetTargetJoinWorld(pName->GetString());
}

void GameMessage_Respawn(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    CharacterData& charData = pPlayer->GetCharData();
    if (!charData.HasCharState(CHAR_STATE_FROZEN))
    {
        pPlayer->OnDeath();
        return;
    }

    pPlayer->SendOnConsoleMessage("I can't respawn right now!");
}

void GameMessage_RespawnSpike(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer || pPlayer->GetCurrentWorld() == 0)
        return;

    auto pTileX = packet.Find("tileX"_hash);
    if (!pTileX)
        return;

    auto pTileY = packet.Find("tileY"_hash);
    if (!pTileY)
        return;

    int32 tileX = 0;
    if (pTileX->GetInt(tileX) != TO_INT_SUCCESS)
        return;

    int32 tileY = 0;
    if (pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    pPlayer->OnDeathSpike(tileX, tileY);
}
