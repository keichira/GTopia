#include "PlayerLoginDetail.h"
#include "Player.h"

#include "../IO/Log.h"

bool PlayerLoginDetail::Serialize(ParsedTextPacket<25>& packet, Player* pPlayer, bool asGameServer)
{
    auto pPlatform = packet.Find(CompileTimeHashString("platformID"));
    if(!pPlatform) {
        return false;
    }
    platformType = (int8)ToInt(string(pPlatform->value, pPlatform->size));

    auto pReqName = packet.Find(CompileTimeHashString("requestedName"));
    if(!pReqName) {
        return false;
    }
    requestedName = string(pReqName->value, pReqName->size);

    auto pProto = packet.Find(CompileTimeHashString("protocol"));
    if(!pProto) {
        return false;
    }
    protocol = (uint32)ToInt(string(pProto->value, pProto->size));

    /*auto pHash = packet.Find(CompileTimeHashString("hash"));
    if(!pHash) {
        return false;
    }
    hash = (uint32)ToInt(string(pHash->value, pHash->size));*/

    auto pTankIDName = packet.Find(CompileTimeHashString("tankIDName"));
    if(pTankIDName) {
        tankIDName = string(pTankIDName->value, pTankIDName->size);
    }

    auto pRid = packet.Find(CompileTimeHashString("rid"));
    if(!pRid) {
        return false;
    }
    rid = string(pRid->value, pRid->size);

#ifdef _DEBUG
    // random generated mac also
    mac = "f4:fb:8f:a9:7a:bd"; // for linux (temp)
    // actually need to fix that for linux based players
    // ain't gonna lie im not sure if its happens only on me
    // mac is required for login system sooo maybe modify client?
    // im lazy for continuing custom client lol
    // because need to do much things for custom client
    // for now i've reached the world select menu in custom client :(
    // i might be yapping too since not much linux users LOLL
#else
    auto pMac = packet.Find(CompileTimeHashString("mac"));
    if(!pMac) {
        return false;
    }
    mac = string(pMac->value, pMac->size);
#endif

    auto pGameVersion = packet.Find(CompileTimeHashString("game_version"));
    if(!pGameVersion) {
        return false;
    }
    gameVersion = ToFloat(string(pGameVersion->value, pGameVersion->size));

    if(asGameServer) {
        auto pToken = packet.Find(CompileTimeHashString("token"));
        if(!pToken) {
            return false;
        }
        token = (uint32)ToInt(string(pToken->value, pToken->size));

        auto pUser = packet.Find(CompileTimeHashString("user"));
        if(!pUser) {
            return false;
        }
        user = (uint32)ToInt(string(pUser->value, pUser->size));
    }

    return true;
}
