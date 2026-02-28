#include "TCPEventWorldInit.h"
#include "../../World/WorldManager.h"

void TCPEventWorldInit::Execute(NetClient* pClient, VariantVector& data)
{
    if(data[1].GetType() == VARIANT_TYPE_BOOL) {
        GetWorldManager()->HandleWorldInit(data[1].GetBool(), data[2].GetUINT());
    }
    else {
        GetWorldManager()->ManagePlayerJoin(data[1].GetUINT(), data[2].GetINT(), data[3].GetString());
    }
}