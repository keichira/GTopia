#include "PlayerLoginDetail.h"
#include "Player.h"
#include "../Proton/ProtonUtils.h"
#include "../IO/Log.h"

bool PlayerLoginDetail::Serialize(ParsedTextPacket<25>& packet, Player* pPlayer, bool asGameServer)
{   
    // platformID,is64bit,hardcoded?
    auto pPlatform = packet.Find(CompileTimeHashString("platformID"));
    string platformStr(pPlatform->value, pPlatform->size);
    if(!pPlatform || ToUInt(platformStr, platformType) != TO_INT_SUCCESS) {
        if(platformStr.find_first_of(",") != string::npos) {
            if(ToUInt(Split(platformStr, ',')[0], platformType) != TO_INT_SUCCESS) {
                return false;
            }
        }
        else {
            return false;
        }
    }

    if(platformType > Proton::PLATFORM_ID_COUNT) {
        LOGGER_LOG_WARN("Unknown platform type %d IP: %s", pPlayer->GetAddress().c_str())
        return false;
    }

    auto pReqName = packet.Find(CompileTimeHashString("requestedName"));
    if(!pReqName) {
        return false;
    }
    requestedName = string(pReqName->value, pReqName->size);

    auto pProto = packet.Find(CompileTimeHashString("protocol"));
    if(!pProto || ToUInt(string(pProto->value, pProto->size), protocol) != TO_INT_SUCCESS) {
        return false;
    }

    auto pHash = packet.Find(CompileTimeHashString("hash"));
    if(!pHash || ToInt(string(pHash->value, pHash->size), hash) != TO_INT_SUCCESS) {
        return false;
    }

    auto pTankIDName = packet.Find(CompileTimeHashString("tankIDName"));
    auto pTankIDPass = packet.Find(CompileTimeHashString("tankIDPass"));

    if((pTankIDName && !pTankIDPass) || (!pTankIDName && pTankIDPass)) {
        return false;
    }

    if(pTankIDName) {
        tankIDName = string(pTankIDName->value, pTankIDName->size);

        if(tankIDName.empty()) {
            return false;
        }
    }

    if(pTankIDPass) {
        tankIDPass = string(pTankIDPass->value, pTankIDPass->size);

        if(tankIDPass.empty()) {
            return false;
        }
    }

    auto pRid = packet.Find(CompileTimeHashString("rid"));
    if(pRid) {
        rid = string(pRid->value, pRid->size);   
    }
    else {
        rid = "11111111111111111111111111111111";
    }

    auto pGid = packet.Find(CompileTimeHashString("gid"));
    if(pGid) {
        gid = string(pGid->value, pGid->size);
        
        if(gid.size() != 36 || gid.empty()) {
            LOGGER_LOG_WARN("Got weird gid (%s) size %d IP: %s", gid.c_str(), gid.size(), pPlayer->GetAddress().c_str());
            return false;
        }
    }

    auto pVid = packet.Find(CompileTimeHashString("vid"));
    if(pVid) {
        vid = string(pVid->value, pVid->size);
        
        if(vid.empty()) {
            LOGGER_LOG_WARN("Got weird vid (%s) size %d IP: %s", gid.c_str(), gid.size(), pPlayer->GetAddress().c_str());
            return false;
        }
    }

#if defined(_DEBUG) && defined(__linux__)
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

mac = "f4:fb:8f:a9:7a:bd";

    if(platformType == Proton::PLATFORM_ID_WINDOWS && (mac.empty() || mac.size() != 17)) {
        LOGGER_LOG_WARN("Got weird mac address (%s) size %d IP: %s", mac.c_str(), mac.size(), pPlayer->GetAddress().c_str());
        return false;
    }

    if(platformType == Proton::PLATFORM_ID_IOS && mac != "02:00:00:00:00:00" && hash == 1431658473) {
        hash = Proton::HashString(mac.c_str(), 0);
    }

    auto pGameVersion = packet.Find(CompileTimeHashString("game_version"));
    if(!pGameVersion) {
        return false;
    }
    gameVersion = ToFloat(string(pGameVersion->value, pGameVersion->size));

    auto pCountry = packet.Find(CompileTimeHashString("country"));
    if(pCountry) {
        country = string(pCountry->value, pCountry->size);
    }

    if(platformType == Proton::PLATFORM_ID_WINDOWS) {
        auto pWk = packet.Find(CompileTimeHashString("wk"));
        if(pWk) {
            sid = string(pWk->value, pWk->size);
    
            if(sid == "NONE0" || sid == "NONE1" || sid == "NONE2") {
                return false;
            }
        }
        else if(gameVersion > 2.17) {
            auto pSid = packet.Find(CompileTimeHashString("sid"));
            if(!pSid) {
                return false;
            }
    
            sid = string(pSid->value, pSid->size);
        }
    }

    if(asGameServer) {
        auto pToken = packet.Find(CompileTimeHashString("token"));
        if(!pToken || ToUInt(string(pToken->value, pToken->size), token) != TO_INT_SUCCESS) {
            return false;
        }

        auto pUser = packet.Find(CompileTimeHashString("user"));
        if(!pUser || ToUInt(string(pUser->value, pUser->size), user) != TO_INT_SUCCESS) {
            return false;
        }

        auto pLMode = packet.Find(CompileTimeHashString("lmode"));
        if(!pLMode || ToUInt(string(pLMode->value, pLMode->size), loginMode) != TO_INT_SUCCESS) {
            return false;
        }
    }

    return true;
}
