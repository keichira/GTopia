#pragma once

#include "../Precompiled.h"
#include "../Utils/Variant.h"
#include "../Proton/ProtonUtils.h"

namespace VariantPacket {
    
    VariantVector OnWelcomePacket(uint32 protocol, float gameVersion, uint32 itemsDatHash, 
                                   const string& cdnServer, const string& cdnPath, 
                                   const string& settings, uint32 tributeHash);
                                   
    VariantVector OnSendToServer(uint16 port, uint32 token, uint32 userID, 
                                    const string& serverIP, int32 logonMode);

    VariantVector OnConsoleMessage(const string& message);
    VariantVector OnRequestWorldSelectMenu(const string& worldMenu);
    VariantVector OnDialogRequest(const string& dialogData);
    VariantVector OnTextOverlay(const string& message);
    VariantVector OnAddNotification(const string& image, const string& message, 
                                       const string& audio, bool isTip);

    VariantVector OnStoreRequest(const string& storeData);
    VariantVector OnStorePurchaseResult(const string& resultText);

    VariantVector OnSetPos(float x, float y);
    VariantVector OnNameChanged(const string& name);
    VariantVector OnFailedToEnterWorld();
    VariantVector OnSpawn(const string& spawnData);
    VariantVector OnRemove(int32 netID);
    VariantVector OnSetCurrentWeather(int32 weatherID);
    VariantVector OnChangeSkin(uint32 skinColor);
    VariantVector OnTalkBubble(uint32 netID, const string& message, bool stackMessages);
    VariantVector SetHasGrowID(bool active, const string& tankIDName, const string& tankIDPass);
    VariantVector OnSetBux(uint32 gemCount, bool skipAnim, bool isSupporter, bool isSuperSupporter, float secondsFromMidnight);
    VariantVector OnDataConfig(bool isMod, bool isSMod);
    VariantVector OnAction(const string& action);
    VariantVector OnPlayPositioned(const string& fileName);
    VariantVector OnParticleEffect(int32 effectType, const Vector2Float& pos, float angle);
    VariantVector OnSetFeatureEnableFlags(const string& str);
}