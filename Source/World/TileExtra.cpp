#include "TileExtra.h"
#include "Item/ItemInfo.h"
#include "TileInfo.h"

uint8 GetTileExtraType(uint8 itemType)
{
    switch (itemType)
    {
        case ITEM_TYPE_USER_DOOR:
        case ITEM_TYPE_DOOR:
        case ITEM_TYPE_PORTAL:
        case ITEM_TYPE_SUNGATE:
            return TILE_EXTRA_TYPE_DOOR;

        case ITEM_TYPE_SIGN:
            return TILE_EXTRA_TYPE_SIGN;

        case ITEM_TYPE_LOCK:
            return TILE_EXTRA_TYPE_LOCK;

        case ITEM_TYPE_SEED:
            return TILE_EXTRA_TYPE_SEED;

        case ITEM_TYPE_MAILBOX:
            return TILE_EXTRA_TYPE_MAILBOX;

        case ITEM_TYPE_BULLETIN:
            return TILE_EXTRA_TYPE_BULLETIN;

        case ITEM_TYPE_PROVIDER:
            return TILE_EXTRA_TYPE_PROVIDER;

        case ITEM_TYPE_ACHIEVEMENT:
            return TILE_EXTRA_TYPE_ACHIEVEMENT;

        case ITEM_TYPE_HEART_MONITOR:
            return TILE_EXTRA_TYPE_HEART_MONITOR;

        case ITEM_TYPE_XENONITE:
            return TILE_EXTRA_TYPE_XENONITE;

        case ITEM_TYPE_CRYSTAL:
            return TILE_EXTRA_TYPE_CRYSTAL;

        case ITEM_TYPE_BATTLE_CAGE:
            return TILE_EXTRA_TYPE_BATTLE_CAGE;

        case ITEM_TYPE_PET_TRAINER:
            return TILE_EXTRA_TYPE_PET_TRAINER;

        case ITEM_TYPE_FIELD_NODE:
            return TILE_EXTRA_TYPE_FIELD_NODE;

        case ITEM_TYPE_OUIJA_BOARD:
            return TILE_EXTRA_TYPE_OUIJA_BOARD;

        default:
            return TILE_EXTRA_TYPE_NONE;
    }
}

TileExtra* CreateTileExtra(uint8 type)
{
    switch (type)
    {
        case TILE_EXTRA_TYPE_DOOR:
            return new TileExtra_Door();

        case TILE_EXTRA_TYPE_SIGN:
            return new TileExtra_Sign();

        case TILE_EXTRA_TYPE_LOCK:
            return new TileExtra_Lock();

        case TILE_EXTRA_TYPE_SEED:
            return new TileExtra_Seed();

        case TILE_EXTRA_TYPE_MAILBOX:
            return new TileExtra_Mailbox();

        case TILE_EXTRA_TYPE_BULLETIN:
            return new TileExtra_Bulletin();

        case TILE_EXTRA_TYPE_COMPONENT:
            return new TileExtra_Component();

        case TILE_EXTRA_TYPE_PROVIDER:
            return new TileExtra_Provider();

        case TILE_EXTRA_TYPE_ACHIEVEMENT:
            return new TileExtra_Achievement();

        case TILE_EXTRA_TYPE_HEART_MONITOR:
            return new TileExtra_HeartMonitor();

        case TILE_EXTRA_TYPE_XENONITE:
            return new TileExtra_Xenonite();

        case TILE_EXTRA_TYPE_CRYSTAL:
            return new TileExtra_Crystal();

        case TILE_EXTRA_TYPE_BATTLE_CAGE:
            return new TileExtra_BattleCage();

        case ITEM_TYPE_PET_TRAINER:
            return new TileExtra_PetTrainer();

        case TILE_EXTRA_TYPE_FIELD_NODE:
            return new TileExtra_FieldNode();

        case TILE_EXTRA_TYPE_OUIJA_BOARD:
            return new TileExtra_OuijaBoard();

        default:
            return nullptr;
    }
}

void TileExtra::Serialize(MemoryBuffer& memBuffer, bool write)
{
    memBuffer.ReadWrite(type, write);
}

void TileExtraGrowth::FinalizeGrowth(uint32 ageMS)
{
    uint32 now = Time::GetSystemTime();
    uint32 elapsedMS = now - timer;
    uint32 elapsedSec = elapsedMS / 1000;
    uint32 correctedTimer = now - (elapsedMS % 1000);

    if (ageMS != 0)
    {
        elapsedSec = ageMS / 1000;
        correctedTimer = timer;
    }

    if (type == TILE_EXTRA_TYPE_TAMAGOTCHI)
        return;

    if (type == TILE_EXTRA_TYPE_FORGE || type == TILE_EXTRA_TYPE_STEAM_ENGINE || type == TILE_EXTRA_TYPE_FOSSIL_PREP)
    {
        if (growTime < elapsedSec)
        {
            growTime = 0;
        }
        else
        {
            growTime -= elapsedSec;
        }

        timer = correctedTimer;
        return;
    }

    growTime += elapsedSec;
    timer = correctedTimer;
}

void TileExtraGrowth::ModGrowth(int32 deltaAgeSec, int32 ageSec)
{
    FinalizeGrowth(0);
    uint32 newGrowTime = 0;

    if (deltaAgeSec < 1)
    {
        if (type == TILE_EXTRA_TYPE_BURGLAR || type == TILE_EXTRA_TYPE_STEAM_ENGINE ||
            type == TILE_EXTRA_TYPE_FOSSIL_PREP)
        {
            newGrowTime = growTime - deltaAgeSec;
        }
        else
        {
            newGrowTime = growTime + deltaAgeSec;

            if (newGrowTime < 0)
            {
                growTime = 0;
                return;
            }
        }
    }
    else
    {
        FinalizeGrowth(deltaAgeSec * 1000);
        newGrowTime = growTime;
    }

    if (ageSec < newGrowTime)
    {
        newGrowTime = ageSec;
    }

    growTime = newGrowTime;
}

float TileExtraGrowth::GetGrowthPercent(TileInfo* pTile)
{
    if (!pTile)
        return 0.0f;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return 0.0f;

    if (pItem->growTime == 0.0f)
    {
        pItem->growTime = 0.0001f;
    }

    FinalizeGrowth(0);
    float progress = (float)growTime / pItem->growTime;

    if (progress > 1.0f)
    {
        progress = 1.0f;
    }

    return progress * 100.0f;
}

void TileExtra_Door::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);

    if (database)
    {
        if (IsMainDoor(pTile->GetFG()))
        {
            name = "";
            text = "EXIT";
            id = "";
        }

        memBuffer.ReadWriteString(name, write);
        memBuffer.ReadWriteString(text, write);
        memBuffer.ReadWriteString(id, write);
    }
    else
    {
        if (write)
        {
            bool isSpecial = false;
            if (pTile && (pTile->GetFG() == ITEM_ID_GATEWAY_TO_ADVENTURE))
            {
                isSpecial = true;
            }

            if (!isSpecial)
            {
                if (name.empty())
                {
                    string temp = text;

                    usize pos = temp.find_first_of(':');
                    if (pos != string::npos)
                    {
                        temp.erase(pos);
                        temp = "...";
                    }

                    memBuffer.ReadWriteString(temp, write);
                }
                else
                {
                    memBuffer.ReadWriteString(name, write);
                }
            }
            else
            {
                memBuffer.ReadWriteString(text, write);
            }
        }
        else
        {
            memBuffer.ReadWriteString(text, write);
        }
    }

    memBuffer.ReadWrite(flags, write);
}

void TileExtra_Sign::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);

    if (pTile && IsPathMarker(pTile->GetFG()))
    {
        if (!database && write)
        {
            string temp;
            memBuffer.ReadWriteString(temp, write);
        }
        else
        {
            memBuffer.ReadWriteString(text, write);
        }
    }
    else
    {
        memBuffer.ReadWriteString(text, write);
    }

    int32 unk = -1; // something with owner union but eh
    memBuffer.ReadWrite(unk, write);
}

void TileExtra_Lock::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWrite(flags, write);
    memBuffer.ReadWrite(ownerID, write);

    uint32 accessSize = accessList.size();
    memBuffer.ReadWrite(accessSize, write);

    if (!write)
    {
        accessList.resize(accessSize);
    }

    if (accessSize > 0)
    {
        memBuffer.ReadWriteRaw(accessList.data(), accessSize * sizeof(int32), write);
    }

    if (worldVersion > 11)
    {
        memBuffer.ReadWrite(minEntryLevel, write);
    }

    if (worldVersion > 12)
    {
        memBuffer.ReadWrite(worldTimer, write);
    }
}

void TileExtra_Seed::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion)
{
    FinalizeGrowth(0);

    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWrite(growTime, write);
    memBuffer.ReadWrite(fruitCount, write);
}

void TileExtra_Component::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile,
                                    uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWrite(randValue, write);
}

void TileExtra_Provider::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile,
                                   uint16 worldVersion)
{
    FinalizeGrowth(0);

    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWrite(growTime, write);

    /*uint16 fgItem = pTile->GetFG();
    if(fgItem == ITEM_ID_WINTERFEST_CALENDAR_2017) {
        memBuffer.ReadWrite(otherData, write);
    }

    if(!database && (fgItem == ITEM_ID_WINTERFEST_CALENDAR_2018 || fgItem == ITEM_ID_WINTERFEST_CALENDAR_2019)) {
        memBuffer.ReadWrite(otherData, write);
    }

    if(!database && fgItem == ITEM_ID_BUILDING_BLOCKS_MACHINE) {
        // long
        // long
    }

    if(worldVersion > 9 &&
        (
            fgItem == ITEM_ID_KEENAN_GTS_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_VENOMSTS_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_LINKTRADER_GTS_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_TERYS_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_EVETS_GTS_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_BENBARRAGES_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_MRSONGO_GTS_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_OLDGERTIES_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_MACPROOF_GTS_AWESOME_ITEM_O_MATIC
        ))
    {
        // long
        // long
        // same as building blocks machine
    }*/
}

void TileExtra_Achievement::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile,
                                      uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWrite(ownerID, write);
    memBuffer.ReadWrite(achievementID, write);
}

void TileExtra_HeartMonitor::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile,
                                       uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWrite(ownerID, write);
    memBuffer.ReadWriteString(playerDisplayName, write);
}

void TileExtra_Xenonite::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile,
                                   uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWrite(flags, write);
    memBuffer.ReadWrite(flags2, write);
}

void TileExtra_OuijaBoard::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile,
                                     uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWrite(playerCount, write);
    memBuffer.ReadWriteString(ouijaType, write);
    memBuffer.ReadWriteString(command, write);

    uint32 itemsSize = items.size();
    memBuffer.ReadWrite(itemsSize, write);

    if (!write)
    {
        items.resize(itemsSize);
    }

    if (itemsSize > 0)
    {
        memBuffer.ReadWriteRaw(items.data(), itemsSize * sizeof(int32), write);
    }
}

void TileExtra_FieldNode::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile,
                                    uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWrite(expireTime, write);

    uint32 nodeSize = nodes.size();
    memBuffer.ReadWrite(nodeSize, write);

    if (!write)
    {
        nodes.resize(nodeSize);
    }

    if (nodeSize > 0)
    {
        memBuffer.ReadWriteRaw(nodes.data(), nodeSize * sizeof(int32), write);
    }
}

void TileExtra_BattleCage::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile,
                                     uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWriteString(cageName, write);

    if (!database)
    {
        int32 baseID = ToItemClientID(basePet);
        int32 secondID = ToItemClientID(secondPet);
        int32 thirdID = ToItemClientID(thirdPet);

        memBuffer.ReadWrite(baseID, write);
        memBuffer.ReadWrite(secondID, write);
        memBuffer.ReadWrite(thirdID, write);
    }
    else
    {
        memBuffer.ReadWrite(basePet, write);
        memBuffer.ReadWrite(secondPet, write);
        memBuffer.ReadWrite(thirdPet, write);
    }
}

void TileExtra_PetTrainer::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile,
                                     uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWriteString(trainerName, write);

    uint32 petSize = pets.size();
    memBuffer.ReadWrite(petSize, write);

    if (!write)
    {
        pets.resize(petSize);
    }

    memBuffer.ReadWrite(unk, write);

    if (petSize > 0)
    {
        memBuffer.ReadWriteRaw(pets.data(), petSize * sizeof(int32), write);
    }

    if (worldVersion > 23)
    {
        memBuffer.ReadWriteString(unk2, write);
    }
}

void TileExtra_Mailbox::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile,
                                  uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);

    if (!database)
    {
        string data;
        memBuffer.ReadWriteString(data, write);
        memBuffer.ReadWriteString(data, write);
        memBuffer.ReadWriteString(data, write);

        uint8 flags = 0;
        memBuffer.ReadWrite(flags, write);
    }
    else
    {
        uint8 letterSize = letters.size();
        memBuffer.ReadWrite(letterSize, write);

        if (!write)
        {
            letters.resize(letterSize);
        }

        for (auto& letter : letters)
        {
            memBuffer.ReadWrite(letter.userID, write);
            memBuffer.ReadWriteString(letter.message, write);
        }
    }
}

void TileExtra_Crystal::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile,
                                  uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWriteString(crystals, write);

    if (database)
    {
        memBuffer.ReadWriteRaw(chi, 4 * sizeof(int16), write);
    }
}

void TileExtra_Bulletin::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile,
                                   uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);

    if (!database)
    {
        string data;
        memBuffer.ReadWriteString(data, write);
        memBuffer.ReadWriteString(data, write);
        memBuffer.ReadWriteString(data, write);

        uint8 flags = 0;
        memBuffer.ReadWrite(flags, write);
    }
    else
    {
        uint8 letterSize = letters.size();
        memBuffer.ReadWrite(letterSize, write);

        if (!write)
        {
            letters.resize(letterSize);
        }

        for (auto& letter : letters)
        {
            memBuffer.ReadWrite(letter.userID, write);
            memBuffer.ReadWriteString(letter.message, write);
        }
    }
}