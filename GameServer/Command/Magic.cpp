#include "Utils/StringUtils.h"
#include "Magic.h"
#include "../World/WorldManager.h"
#include "Math/Random.h"
#include "Math/Math.h"

const CommandInfo& Magic::GetInfo()
{
    static CommandInfo info =
    {
        "/magic",
        "Bass da da da",
        ROLE_PERM_COMMAND_MAGIC,
        {
            CompileTimeHashString("magic")
        }
    };

    return info;
}

void Magic::Execute(GamePlayer* pPlayer, std::vector<string>& args)
{
    if(!pPlayer || args.empty() || !CheckPerm(pPlayer)) {
        return;
    }

    if(pPlayer->GetCurrentWorld() == 0) {
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByID(pPlayer->GetCurrentWorld());
    if(!pWorld) {
        return;
    }

    pWorld->PlaySFXForEveryone("magic.wav");

    Vector2Float playerPos = pPlayer->GetWorldPos();

    for(uint8 i = 0; i < 15; ++i) {
        float offsetX = RandomRangeFloat(-80.0f, 100.0f);
        float offsetY = RandomRangeFloat(-80.0f, 100.0f);

        int32 particleType = RandomRangeInt(0, 3);
        pWorld->SendParticleEffectToAll(playerPos.x + offsetX, playerPos.y + offsetY, particleType, 4, 150 * i);
    }
}
