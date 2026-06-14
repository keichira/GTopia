#include "PlayerLoginDetail.h"
#include "Player.h"
#include "../Proton/ProtonUtils.h"
#include "../IO/Log.h"
#include "../Utils/Base64.h"

bool PlayerLoginDetail::Serialize(ParsedTextPacket<35>& packet, Player* pPlayer, bool asGameServer)
{   
    if(!pPlayer) 
        return false;
    
    std::string_view playerIP = pPlayer->GetAddress();

    auto pPlatform = packet.Find("platformID"_hash);
    if (!pPlatform) 
    {
        LOGGER_LOG_WARN("[LOGIN_FAIL] PlatformID field missing. IP: %s", playerIP.data());
        return false;
    }

    string platformStr(pPlatform->value, pPlatform->size);
    if(ToUInt(platformStr, platformType) != TO_INT_SUCCESS) 
    {
        if(platformStr.find_first_of(",") != string::npos)
        {
            if(ToUInt(Split(platformStr, ',')[0], platformType) != TO_INT_SUCCESS) 
            {
                LOGGER_LOG_WARN("[LOGIN_FAIL] Malformed platformID string '%s'. IP: %s", platformStr.c_str(), playerIP.data());
                return false;
            }
        }
        else 
        {
            LOGGER_LOG_WARN("[LOGIN_FAIL] Invalid platformID format '%s'. IP: %s", platformStr.c_str(), playerIP.data());
            return false;
        }
    }

    if(platformType > Proton::PLATFORM_ID_COUNT) 
    {
        LOGGER_LOG_WARN("[LOGIN_FAIL] Unknown/Spoofed platform type %d. IP: %s", platformType, playerIP.data());
        return false;
    }

    auto pProto = packet.Find("protocol"_hash);
    if(!pProto || ToUInt(string(pProto->value, pProto->size), protocol) != TO_INT_SUCCESS) 
    {
        LOGGER_LOG_WARN("[LOGIN_FAIL] Missing or invalid protocol version. IP: %s", playerIP.data());
        return false;
    }

    if(!asGameServer && protocol > 200)
    {
        auto pLToken = packet.Find("ltoken"_hash);
        if(!pLToken) 
        {
            LOGGER_LOG_WARN("[LOGIN_FAIL] Protocol (%d) but ltoken is missing. IP: %s", protocol, playerIP.data());
            return false;
        }

        if(pLToken->size < 100) 
        {
            LOGGER_LOG_WARN("[LOGIN_FAIL] Suspiciously small ltoken size (%d). IP: %s", pLToken->size, playerIP.data());
            return false;
        }

        string payload;
        if(!Base64_Decode((void*)pLToken->value, pLToken->size, payload)) 
        {
            LOGGER_LOG_WARN("[LOGIN_FAIL] Base64 decode failed for ltoken. IP: %s", playerIP.data());
            return false;
        }

        if(payload.empty()) return false;

        // Payload ayrıştırma kontrolleri
        usize loginInfoPos = payload.find("loginInfo=");
        usize growIDPos = payload.find("&growID=");
        usize passwordPos = payload.find("&password=");

        if(loginInfoPos == string::npos || growIDPos == string::npos || passwordPos == string::npos) 
        {
            LOGGER_LOG_WARN("[LOGIN_FAIL] ltoken payload layout is invalid/corrupted. IP: %s", playerIP.data());
            return false;
        }

        usize loginInfoStart = loginInfoPos + 10;
        string loginInfo = payload.substr(loginInfoStart, growIDPos - loginInfoStart);
    
        if(loginInfo.empty()) 
        {
            LOGGER_LOG_WARN("[LOGIN_FAIL] ltoken contains empty loginInfo. IP: %s", playerIP.data());
            return false;
        }

        usize growIDStart = growIDPos + 8;
        usize passwordStart = passwordPos + 10;

        tankIDPass = payload.substr(passwordStart);

        if(!tankIDPass.empty()) 
        {
            tankIDName = payload.substr(growIDStart, passwordPos - growIDStart);
            if(tankIDName.empty()) return false;
        } 
        else 
        {
            requestedName = payload.substr(growIDStart, passwordPos - growIDStart);
            tankIDPass = "";
        }

        ParseTextPacket(loginInfo.data(), loginInfo.size(), packet);
    }

    if(protocol <= 200)
    {
        auto pReqName = packet.Find("requestedName"_hash);
        if(!pReqName) 
        {
            LOGGER_LOG_WARN("[LOGIN_FAIL] Legacy protocol but requestedName missing. IP: %s", playerIP.data());
            return false;
        }
        requestedName = string(pReqName->value, pReqName->size);
    
        auto pTankIDName = packet.Find("tankIDName"_hash);
        auto pTankIDPass = packet.Find("tankIDPass"_hash);
    
        if((pTankIDName && !pTankIDPass) || (!pTankIDName && pTankIDPass)) 
        {
            LOGGER_LOG_WARN("[LOGIN_FAIL] Mismatched TankID credentials (one field missing). Name: %s, IP: %s", requestedName.c_str(), playerIP.data());
            return false;
        }
    
        if(pTankIDName) tankIDName = string(pTankIDName->value, pTankIDName->size);
        if(pTankIDPass) tankIDPass = string(pTankIDPass->value, pTankIDPass->size);
    }

    auto pHash = packet.Find("hash"_hash);
    if(!pHash || ToInt(string(pHash->value, pHash->size), hash) != TO_INT_SUCCESS) 
    {
        LOGGER_LOG_WARN("[LOGIN_FAIL] Missing or malformed login hash. IP: %s", playerIP.data());
        return false;
    }

    auto pRid = packet.Find("rid"_hash);
    rid = pRid ? string(pRid->value, pRid->size) : "11111111111111111111111111111111";

    auto pGid = packet.Find("gid"_hash);
    if(pGid) {
        gid = string(pGid->value, pGid->size);
        if(gid.size() != 36 || gid.empty()) {
            LOGGER_LOG_WARN("[LOGIN_FAIL] Weird GID size (%zu) value: '%s'. IP: %s", gid.size(), gid.c_str(), playerIP.data());
            return false;
        }
    }

    auto pVid = packet.Find("vid"_hash);
    if(pVid) {
        vid = string(pVid->value, pVid->size);
        if(vid.empty()) {
            LOGGER_LOG_WARN("[LOGIN_FAIL] Empty VID field. IP: %s", playerIP.data());
            return false;
        }
    }

#if defined(_DEBUG) && defined(__linux__)
    mac = "f4:fb:8f:a9:7a:bd"; 
#else
    auto pMac = packet.Find("mac"_hash);
    if(!pMac) 
    {
        LOGGER_LOG_WARN("[LOGIN_FAIL] MAC address field missing. IP: %s", playerIP.data());
        return false;
    }
    mac = string(pMac->value, pMac->size);
#endif

    if(platformType == Proton::PLATFORM_ID_WINDOWS && (mac.empty() || mac.size() != 17)) 
    {
        LOGGER_LOG_WARN("[LOGIN_FAIL] Malformed Windows MAC address '%s' (Len: %zu). Player: %s, IP: %s", mac.c_str(), mac.size(), tankIDName.c_str(), playerIP.data());
        return false;
    }

    if(platformType == Proton::PLATFORM_ID_IOS && mac != "02:00:00:00:00:00" && hash == 1431658473) 
    {
        hash = Proton::HashString(mac.c_str(), 0);
    }

    auto pGameVersion = packet.Find("game_version"_hash);
    if(!pGameVersion) 
    {
        LOGGER_LOG_WARN("[LOGIN_FAIL] game_version field missing. IP: %s", playerIP.data());
        return false;
    }
    gameVersion = ToFloat(string(pGameVersion->value, pGameVersion->size));

    auto pCountry = packet.Find("country"_hash);
    if(pCountry) 
    {
        if(pCountry->size > 2 || pCountry->size < 0) 
        {
            LOGGER_LOG_WARN("[LOGIN_FAIL] Invalid country code size (%d). IP: %s", pCountry->size, playerIP.data());
            return false;
        }
        country = string(pCountry->value, pCountry->size);
    }

    if(platformType == Proton::PLATFORM_ID_WINDOWS) 
    {
        auto pWk = packet.Find("wk"_hash);
        if(pWk) 
        {
            sid = string(pWk->value, pWk->size);
            if(sid == "NONE0" || sid == "NONE1" || sid == "NONE2") 
            {
                LOGGER_LOG_WARN("[LOGIN_FAIL] Blocked exploit/default SID value '%s'. IP: %s", sid.c_str(), playerIP.data());
                return false;
            }
        }
        else if(gameVersion > 2.17) 
        {
            auto pSid = packet.Find("sid"_hash);
            if(!pSid) 
            {
                LOGGER_LOG_WARN("[LOGIN_FAIL] Missing SID for Windows client v%.2f. IP: %s", gameVersion, playerIP.data());
                return false;
            }
            sid = string(pSid->value, pSid->size);
        }
    }

    if(asGameServer) 
    {
        auto pToken = packet.Find("token"_hash);
        auto pUser = packet.Find("user"_hash);
        auto pLMode = packet.Find("lmode"_hash);

        if(!pToken || ToUInt(string(pToken->value, pToken->size), token) != TO_INT_SUCCESS ||
           !pUser || ToUInt(string(pUser->value, pUser->size), user) != TO_INT_SUCCESS ||
           !pLMode || ToUInt(string(pLMode->value, pLMode->size), loginMode) != TO_INT_SUCCESS)
        {
            LOGGER_LOG_ERROR("[LOGIN_FAIL] Sub-server login token handshake failure. IP: %s", playerIP.data());
            return false;
        }
    }

    LOGGER_LOG_INFO("[LOGIN_SUCCESS] GrowID: '%s', Ver: %.2f, Platform: %d, Country: %s, IP: %s",
        tankIDName.empty() ? requestedName.c_str() : tankIDName.c_str(),
        gameVersion, platformType, country.c_str(), playerIP.data());

    return true;
}