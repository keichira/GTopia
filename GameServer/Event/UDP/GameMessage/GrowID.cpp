#include "GrowID.h"
#include "Proton/ProtonUtils.h"
#include "Database/Table/PlayerDBTable.h"
#include "../../../Context.h"

void GrowID::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    if(!pPlayer || pPlayer->HasGrowID()) {
        return;
    }

    pPlayer->CheckLimitsForAccountCreation(false);
}