#include "SteamManager.h"
#include "World/World.h"

// ehh its common used just throw here...
int32 STEAM_DIR_X[4] = {1, 0, -1, 0};
int32 STEAM_DIR_Y[4] = {0, 1, 0, -1};

int32 EncodeDirectionFlags(uint8 direction)
{
    return ((direction & 1) ? TILE_FLAG_FG_ALT_MODE : 0) | TILE_FLAG_IS_ON |
           ((direction & 2) ? TILE_FLAG_FLIPPED_X : 0);
}

uint8 DecodeDirectionFlags(TileInfo* pTile)
{
    uint8 direction = 0;
    if (pTile->HasFlag(TILE_FLAG_FG_ALT_MODE))
        direction += 1;

    if (pTile->HasFlag(TILE_FLAG_FLIPPED_X))
        direction += 2;
    return direction;
}

SteamManager::SteamManager(World* pWorld) : m_pWorld(pWorld) {}

void SteamManager::AddNode(int32 x, int32 y, uint8 direction)
{
    int32 bestIndex = 0;
    for (int32 index = 0; index < 64; ++index)
    {
        if (!m_nodes[index].IsAlive())
        {
            bestIndex = index;
            break;
        }

        if (m_nodes[index].lifeTime < m_nodes[bestIndex].lifeTime)
        {
            bestIndex = index;
        }
    }

    SteamNode& node = m_nodes[bestIndex];
    node.x = x;
    node.y = y;
    node.direction = direction;
    node.lifeTime = 100;
    node.cooldown = 0;
}

void SteamManager::KillNode(TileInfo* pTile)
{
    if (!pTile)
        return;

    pTile->SetFlag(TILE_FLAG_WAS_SPLICED);
}

int32 SteamManager::GetWaitingNodeCountAt(TileInfo* pTile)
{
    if (!pTile)
        return 0;

    Vector2Int& vTilePos = pTile->GetPos();
    int32 count = 0;

    for (auto& node : m_nodes)
    {
        if (node.IsWaiting() && node.x == vTilePos.x && node.y == vTilePos.y)
            count++;
    }

    return count;
}

bool SteamManager::HasAliveNodes()
{
    for (auto& node : m_nodes)
    {
        if (node.IsAlive())
            return true;
    }

    return false;
}

void SteamManager::Update()
{
    if (!m_pWorld)
        return;

    if (!HasAliveNodes())
    {
        m_updateTimer.Reset();
        return;
    }

    uint64 elapsedTime = m_updateTimer.GetElapsedTime();
    if (elapsedTime < 100)
        return;

    int32 tickCount = 1;
    if (elapsedTime >= 1000)
    {
        m_updateTimer.Reset();
    }
    else
    {
        tickCount = elapsedTime / 100;
        m_updateTimer.Reset(m_updateTimer.GetStartTime() + tickCount * 100);
    }

    for (int32 tick = 0; tick < tickCount; ++tick)
    {
        Tick();
    }
}

void SteamManager::Tick()
{
    WorldTileManager* pTileMgr = m_pWorld->GetTileManager();

    for (auto& node : m_nodes)
    {
        if (!node.IsAlive())
            continue;

        TileInfo* pTile = pTileMgr->GetTile(node.x, node.y);
        if (node.cooldown == 0)
        {
            ProcessNode(node, pTile);
            continue;
        }

        if (pTile && pTile->HasFlag(TILE_FLAG_WAS_SPLICED))
        {
            node.cooldown = 0;
            node.lifeTime = 0;
            pTile->RemoveFlag(TILE_FLAG_WAS_SPLICED);
            continue;
        }

        if (pTile && pTile->HasFlag(TILE_FLAG_IS_ON))
        {
            int32 tileFg = pTile->GetFG();

            if (tileFg == ITEM_ID_STEAM_VALVE || tileFg == ITEM_ID_STEAM_SCRAMBLER || tileFg == ITEM_ID_STEAM_COLLECTOR)
            {
                node.cooldown = 0;
                ProcessNode(node, pTile);
                continue;
            }
        }

        node.cooldown--;
        if (node.cooldown == 0)
        {
            node.lifeTime = 0;
        }
    }
}

bool SteamManager::IsDirectionOpen(TileInfo* pTarget, int32 direction)
{
    if (!pTarget || !pTarget->IsAcceptSteam())
        return false;

    int32 targetFg = pTarget->GetFG();

    switch (targetFg)
    {
        case ITEM_ID_STEAM_LAMP:
        case ITEM_ID_STEAM_SPIKES:
            return direction == 3;

        case ITEM_ID_STEAM_CRANK:
            if (pTarget->HasFlag(TILE_FLAG_IS_ON))
                return direction == 0 || direction == 2;
            return direction == 1 || direction == 3;

        case ITEM_ID_STEAM_VALVE:
        {
            bool isFlipped = pTarget->HasFlag(TILE_FLAG_FLIPPED_X);
            if (direction == 0 && !isFlipped)
                return false;
            if (direction == 2 && isFlipped)
                return false;
            return true;
        }

        case ITEM_ID_STEAM_VENT:
            return !(direction == 0 || direction == 2);

        case ITEM_ID_SPIRIT_STORAGE_UNIT:
            return direction != 1;
    }

    return true;
}

int32 SteamManager::ComputeOpenDirections(SteamNode& node, bool canGo[4])
{
    WorldTileManager* pTileMgr = m_pWorld->GetTileManager();
    int32 openCount = 0;

    for (int32 direction = 0; direction < 4; ++direction)
    {
        TileInfo* pTarget = pTileMgr->GetTile(node.x + STEAM_DIR_X[direction], node.y + STEAM_DIR_Y[direction]);
        canGo[direction] = IsDirectionOpen(pTarget, direction);

        if (canGo[direction])
        {
            openCount++;
        }
    }

    return openCount;
}

void SteamManager::MoveNode(SteamNode& node, uint8 direction, bool canGo[4])
{
    uint8 nextDir = direction;

    if (!canGo[nextDir])
    {
        nextDir = (nextDir + 1) & 3;
        if (!canGo[nextDir])
        {
            nextDir = (nextDir + 2) & 3;
            if (!canGo[nextDir])
            {
                node.lifeTime = 0;
                return;
            }
        }
    }

    node.x += STEAM_DIR_X[nextDir];
    node.y += STEAM_DIR_Y[nextDir];
    node.direction = nextDir;

    if (node.lifeTime > 0)
    {
        node.lifeTime--;
    }
}

void SteamManager::ReleaseCollectedNodes(TileInfo* pTile, bool canGo[4], uint8 excludedDirection)
{
    Vector2Int& vTilePos = pTile->GetPos();
    int32 nextDir = 0;

    for (auto& other : m_nodes)
    {
        if (!other.IsWaiting() || other.x != vTilePos.x || other.y != vTilePos.y)
            continue;

        while (nextDir < 4 && (nextDir == excludedDirection || !canGo[nextDir]))
        {
            nextDir++;
        }

        other.cooldown = 0;

        if (nextDir >= 4)
        {
            other.lifeTime = 0;
            continue;
        }

        other.direction = nextDir;
        other.x += STEAM_DIR_X[nextDir];
        other.y += STEAM_DIR_Y[nextDir];
        other.lifeTime--;
        nextDir++;
    }
}

void SteamManager::ProcessNode(SteamNode& node, TileInfo* pTile)
{
    bool canGo[4];
    int32 openCount = ComputeOpenDirections(node, canGo);

    uint8 direction = node.direction;
    uint8 backDirection = (direction + 2) & 3;

    if (!pTile)
    {
        MoveNode(node, direction, canGo);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();
    bool isFlipped = pTile->HasFlag(TILE_FLAG_FLIPPED_X);
    bool isOn = pTile->HasFlag(TILE_FLAG_IS_ON);

    switch (pTile->GetFG())
    {
        case ITEM_ID_STEAM_VENT:
            m_pWorld->SendSteamPacketToAll(STEAM_EVENT_VENT, vTilePos.x, vTilePos.y);
            break;

        case ITEM_ID_STEAM_LAMP:
        {
            node.lifeTime = 0;
            m_pWorld->SendSteamPacketToAll(STEAM_EVENT_LAMP, vTilePos.x, vTilePos.y);
            return;
        }

        case ITEM_ID_STEAM_SPIKES:
        {
            node.lifeTime = 0;
            pTile->ToggleFlag(TILE_FLAG_IS_ON);
            m_pWorld->SendSteamPacketToAll(pTile->HasFlag(TILE_FLAG_IS_ON) ? STEAM_EVENT_SPIKES_ON
                                                                           : STEAM_EVENT_SPIKES_OFF,
                                           vTilePos.x, vTilePos.y);
            return;
        }

        case ITEM_ID_STEAM_DOOR:
        {
            node.lifeTime = 0;
            pTile->ToggleFlag(TILE_FLAG_IS_ON);
            m_pWorld->SendSteamPacketToAll(pTile->HasFlag(TILE_FLAG_IS_ON) ? STEAM_EVENT_DOOR_OPEN
                                                                           : STEAM_EVENT_DOOR_CLOSE,
                                           vTilePos.x, vTilePos.y);
            return;
        }

        case ITEM_ID_STEAM_LAUNCHER:
        {
            node.lifeTime = 0;
            pTile->SetFlag(TILE_FLAG_IS_ON);
            m_pWorld->SendSteamPacketToAll(STEAM_EVENT_LAUNCHER_CHARGE, vTilePos.x, vTilePos.y);
            return;
        }

        case ITEM_ID_STEAM_CROSSOVER:
        {
            uint8 turnedDir = isFlipped ? 3 - direction : direction ^ 1;
            bool turnedOpen = canGo[turnedDir];

            canGo[0] = canGo[1] = canGo[2] = canGo[3] = false;
            canGo[turnedDir] = turnedOpen;
            direction = turnedDir;
            break;
        }

        case ITEM_ID_STEAM_CRANK:
        {
            if (isOn)
                canGo[1] = canGo[3] = false;
            else
                canGo[0] = canGo[2] = false;
            break;
        }

        case ITEM_ID_STEAM_FUNNEL:
        {
            canGo[1] = canGo[3] = false;
            if (isFlipped)
            {
                canGo[0] = false;
                direction = 2;
            }
            else
            {
                canGo[2] = false;
                direction = 0;
            }
            break;
        }

        case ITEM_ID_STEAM_FUNNEL_UP:
        {
            canGo[0] = canGo[1] = canGo[2] = false;
            direction = 3;
            break;
        }

        case ITEM_ID_STEAM_FUNNEL_DOWN:
        {
            canGo[0] = canGo[2] = canGo[3] = false;
            direction = 1;
            break;
        }

        case ITEM_ID_STEAM_VALVE:
        {
            if (!isOn)
            {
                node.cooldown = 30;
                m_pWorld->SendSteamPacketToAll(isFlipped ? STEAM_EVENT_FLIP_VALVE_NO : STEAM_EVENT_FLIP_VALVE_YES,
                                               vTilePos.x, vTilePos.y);
                pTile->ToggleFlag(TILE_FLAG_IS_ON);
                return;
            }

            pTile->RemoveFlag(TILE_FLAG_IS_ON);
            if (isFlipped)
                canGo[0] = false;
            else
                canGo[2] = false;

            if ((node.direction == 1 || node.direction == 3) && (canGo[0] || canGo[2]))
            {
                direction = canGo[0] ? 0 : 2;
            }

            pTile->ToggleFlag(TILE_FLAG_FLIPPED_X);
            break;
        }

        case ITEM_ID_STEAM_SCRAMBLER:
        {
            if (!isOn)
            {
                node.cooldown = 30;

                if (openCount < 2)
                {
                    m_pWorld->SendSteamPacketToAll(STEAM_EVENT_FAIL, vTilePos.x, vTilePos.y);
                    KillNode(pTile);
                    return;
                }

                uint8 scrambledDir = RandomRangeInt(0, 3);
                while (scrambledDir == backDirection || !canGo[scrambledDir])
                {
                    scrambledDir = RandomRangeInt(0, 3);
                }

                m_pWorld->SendSteamPacketToAll((eSteamEvent)(STEAM_EVENT_SCRAMBLE_RIGHT + scrambledDir), vTilePos.x,
                                               vTilePos.y);
                pTile->RemoveFlag(TILE_FLAG_FLIPPED_X | TILE_FLAG_FG_ALT_MODE);
                pTile->SetFlag(EncodeDirectionFlags(scrambledDir));
                return;
            }

            direction = DecodeDirectionFlags(pTile);
            pTile->RemoveFlag(TILE_FLAG_IS_ON);
            break;
        }

        case ITEM_ID_STEAM_COLLECTOR:
        {
            node.cooldown = 200;

            int32 waitingCount = GetWaitingNodeCountAt(pTile);
            int32 neededCount = openCount;
            if (!canGo[backDirection])
                neededCount++;

            if (neededCount > waitingCount + 1)
                return;

            if (!isOn)
            {
                m_pWorld->SendSteamPacketToAll((eSteamEvent)(STEAM_EVENT_COLLECTED_NOT_RIGHT + backDirection),
                                               vTilePos.x, vTilePos.y);
                pTile->RemoveFlag(TILE_FLAG_FLIPPED_X | TILE_FLAG_FG_ALT_MODE);
                pTile->SetFlag(EncodeDirectionFlags(backDirection));
                return;
            }

            uint8 excludedDirection = DecodeDirectionFlags(pTile);
            pTile->RemoveFlag(TILE_FLAG_IS_ON);
            ReleaseCollectedNodes(pTile, canGo, excludedDirection);
            return;
        }
    }

    MoveNode(node, direction, canGo);
}