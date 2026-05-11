#include "WrenchSelfDialog.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"
#include "../../World/WorldManager.h"
#include "Math/Math.h"

void WrenchSelfDialog::Request(GamePlayer* pPlayer)
{
    if(!pPlayer)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    PlayerProgress& progressData = pPlayer->GetProgressData();
    uint32 currentLevel = Sqrt((progressData.GetProgress(PLAYER_PROGRESS_XP)/50) - 2);
    uint32 xpToLevelUP = 50 * ((currentLevel + 1) * (currentLevel + 1) + 2);

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddPlayerInfo(pPlayer->GetDisplayName(), currentLevel, progressData.GetProgress(PLAYER_PROGRESS_XP), xpToLevelUP)
    ->EndDialog("wrenchself", "", "Continue");

    pPlayer->SendOnDialogRequest(db.Get());
}