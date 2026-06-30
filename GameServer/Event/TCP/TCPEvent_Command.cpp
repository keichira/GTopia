#include "TCPEvent_Command.h"
#include "../../Player/PlayerManager.h"
#include "../../World/WorldManager.h"
#include "Player/RoleManager.h"

void TCPEvent_Command(NetClient* pClient, VariantVector& data)
{
    if (!pClient)
        return;

    int32 commandType = data[1].GetINT();

    switch (commandType)
    {
        case TCP_COMMAND_SETROLE:
        {
            TCPEvent_Command_SetRole(std::move(data));
            break;
        }
    }
}

void TCPEvent_Command_SetRole(VariantVector&& data)
{
    if (data.size() < 4)
        return;

    uint32 userID = data[2].GetUINT();
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByUserID(userID);
    if (!pPlayer)
        return;

    uint32 roleID = data[3].GetUINT();
    Role* pRole = GetRoleManager()->GetRole(roleID);
    if (!pRole)
        return;

    pPlayer->SetRole(pRole);

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    pWorld->SendPlayerDataConfigToAll(pPlayer);
    pWorld->SendNameChangeToAll(pPlayer);
}
