#include "SetSkin.h"
#include "Utils/StringUtils.h"

void SetSkin::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    auto pColor = packet.Find("color"_hash);
    if(!pColor)
        return;

    uint32 skinColor = 0;
    /*really need to convert to string?*/
    if(ToUInt(string(pColor->value, pColor->size), skinColor) != TO_INT_SUCCESS)
        return;

    //pPlayer->SetSkinColor(skinColor);
}