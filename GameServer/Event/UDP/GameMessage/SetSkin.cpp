#include "SetSkin.h"
#include "Utils/StringUtils.h"

void SetSkin::Execute(GamePlayer* pPlayer, ParsedTextPacket<40>& packet)
{
    auto pColor = packet.Find("color"_hash);
    if(!pColor)
        return;

    uint32 skinColor = 0;
    if(pColor->GetUInt(skinColor) != TO_INT_SUCCESS)
        return;

    //pPlayer->SetSkinColor(skinColor);
}