#include "SetSkin.h"
#include "Utils/StringUtils.h"

void SetSkin::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    auto pColor = packet.Find(CompileTimeHashString("color"));
    if(!pColor) {
        return;
    }

    uint32 skinColor = 0;
    /*really need to convert to string?*/
    if(ToUInt(string(pColor->value, pColor->size), skinColor) != TO_INT_SUCCESS) {
        return;
    }

    /**
     * check if player can set the skin color
     * do not allow other colors
     * or check supporter
     */
    pPlayer->GetCharData().SetSkinColor(skinColor);
}