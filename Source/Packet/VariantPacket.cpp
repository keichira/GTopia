#include "VariantPacket.h"

namespace VariantPacket {

    VariantVector OnWelcomePacket(uint32 protocol, float gameVersion, uint32 itemsDatHash, 
                                   const string& cdnServer, const string& cdnPath, 
                                   const string& settings, uint32 tributeHash)
    {
        VariantVector data(protocol < 93 ? 6 : 7);
        
        string osmHeader;
        if(gameVersion <= 2.982f) {
            if(gameVersion <= 2.479f) {
                if(gameVersion <= 2.459f) {
                    if(2.449f < gameVersion) osmHeader = "OnSuperMainStartAcceptLogonFB211131d";
                } else osmHeader = "OnSuperMainStartAcceptLogonFB211131dd";
            } else osmHeader = "OnSuperMainStartAcceptLogonFB211131ddf";
        } else osmHeader = "OnSuperMainStartAcceptLogonHrdxs47254722215a";

        data[0] = osmHeader;
        data[1] = itemsDatHash;
        data[2] = cdnServer;
        data[3] = cdnPath;
        data[4] = "";
        data[5] = settings;
        
        if(protocol > 93) {
            data[6] = tributeHash;
        }
        return data;
    }

    VariantVector OnSendToServer(uint16 port, uint32 token, uint32 userID, const string& serverIP, int32 logonMode)
    {
        VariantVector data(6);
        data[0] = "OnSendToServer";
        data[1] = (uint32)port;
        data[2] = token;
        data[3] = userID;
        data[4] = serverIP + "||";
        data[5] = logonMode;
        return data;
    }

    VariantVector OnConsoleMessage(const string& message) {
        return { "OnConsoleMessage", message };
    }

    VariantVector OnRequestWorldSelectMenu(const string& worldMenu) {
        return { "OnRequestWorldSelectMenu", worldMenu };
    }

    VariantVector OnDialogRequest(const string& dialogData) {
        return { "OnDialogRequest", dialogData };
    }

    VariantVector OnTextOverlay(const string& message) {
        return { "OnTextOverlay", message };
    }

    VariantVector OnSetPos(float x, float y) {
        return { "OnSetPos", Vector2Float(x, y) };
    }

    VariantVector OnNameChanged(const string &name)
    {
        return { "OnNameChanged", name };
    }

    VariantVector OnFailedToEnterWorld()
    {
        return { "OnFailedToEnterWorld" };
    }

    VariantVector OnSpawn(const string& spawnData) {
        return { "OnSpawn", spawnData };
    }

    VariantVector OnRemove(int32 netID) {
        return { "OnRemove", "netID|" + std::to_string(netID) + "\n" };
    }

    VariantVector OnSetCurrentWeather(int32 weatherID) {
        return { "OnSetCurrentWeather", weatherID };
    }

    VariantVector OnChangeSkin(uint32 skinColor) {
        return { "OnChangeSkin", skinColor };
    }

    VariantVector OnTalkBubble(uint32 netID, const string& message, bool stackMessages)
    {
        VariantVector data(5);
        data[0] = "OnTalkBubble";
        data[1] = netID;
        data[2] = message;
        data[3] = (uint32)0; 
        data[4] = stackMessages ? (uint32)1 : (uint32)0;
        return data;
    }

    VariantVector SetHasGrowID(bool active, const string& tankIDName, const string& tankIDPass)
    {
        VariantVector data(4);
        data[0] = "SetHasGrowID";
        data[1] = active ? 1 : 0;
        data[2] = tankIDName;
        data[3] = tankIDPass;
        return data;
    }

    VariantVector OnSetBux(uint32 gemCount, bool skipAnim, bool isSupporter, bool isSuperSupporter, float secondsFromMidnight)
    {
        VariantVector data(5);
        data[0] = "OnSetBux";
        data[1] = gemCount;
        data[2] = skipAnim ? 1 : 0;
        data[3] = isSupporter ? 1 : 0;
        data[4] = Vector3Float(secondsFromMidnight, isSuperSupporter ? 1 : 0, 0);
        return data;
    }

    VariantVector OnDataConfig(bool isMod, bool isSMod) {
        return { "OnDataConfig", isMod ? 1 : 0, isSMod ? 1 : 0 };
    }

    VariantVector OnAction(const string& action) {
        return { "OnAction", action };
    }

    VariantVector OnPlayPositioned(const string& fileName) {
        return { "OnPlayPositioned", "audio/" + fileName };
    }

    VariantVector OnParticleEffect(int32 effectType, const Vector2Float& pos, float angle) {
        return { "OnParticleEffect", effectType, pos, angle };
    }

    VariantVector OnSetFeatureEnableFlags(const string& str)
    {
        return { "OnSetFeatureEnableFlags", str };
    }

    VariantVector OnStoreRequest(const string& storeData) {
        return { "OnStoreRequest", storeData };
    }

    VariantVector OnStorePurchaseResult(const string& resultText) {
        return { "OnStorePurchaseResult", resultText };
    }

    VariantVector OnAddNotification(const string& image, const string& message, const string& audio, bool isTip)
    {
        VariantVector data(5);
        data[0] = "OnAddNotification";
        data[1] = image;
        data[2] = message;
        data[3] = audio;
        data[4] = (uint32)(isTip ? 1 : 0);
        return data;
    }
}