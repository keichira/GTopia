#include "UserCacheManager.h"
#include "../Player/PlayerManager.h"
#include "../Player/GamePlayer.h"
#include "../Context.h"
#include "Item/ItemInfoManager.h"
#include "Player/RoleManager.h"
#include "../World/WorldManager.h"
#include "../Player/Dialog/LockDialog.h"

UserCacheManager::UserCacheManager()
{
    m_lastCleanupTime.Reset();
}

UserCacheManager::~UserCacheManager()
{
}

void UserCacheManager::Init(uint32 maxExpectedPlayers)
{
    m_pendingRequests.reserve(maxExpectedPlayers);
    m_cache.reserve(maxExpectedPlayers * 2);
    m_inFlightUserIDs.reserve(32);
}

UserMetadata* UserCacheManager::GetMetadata(uint32 userID)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByUserID(userID);
    if(pPlayer)
    {
        uint32 roleID = 0;
        if(Role* pRole = pPlayer->GetRole())
        {
            roleID = pRole->GetID();
        }

        m_cache[userID] = { userID, pPlayer->GetRawName(), pPlayer->GetDisplayName(false), roleID, Timer() };
        return &m_cache[userID];
    }

    auto it = m_cache.find(userID);
    if(it != m_cache.end())
    {
        it->second.cacheTime.Reset();
        return &it->second;
    }
    return nullptr;
}

bool UserCacheManager::IsCached(uint32 userID)
{
    return GetMetadata(userID) != nullptr;
}

void UserCacheManager::FetchMetadata(uint32 playerNetID, 
                                    eCacheRequestType reqType,
                                    const std::vector<int32>& userIDs, 
                                    std::array<CacheParam, 5> params, 
                                    const char* textParam)
{
    if(m_pendingRequests.find(playerNetID) != m_pendingRequests.end())
        return;

    static std::vector<int32> missingIDs;
    missingIDs.clear(); 

    for(int32 id : userIDs) 
    {
        if(id > 0 && !IsCached(id)) 
        {
            missingIDs.push_back(id);
        }
    }

    if(missingIDs.empty())
    {
        PendingRequest fastRequest;
        fastRequest.reqType = reqType;
        fastRequest.params = params;
        fastRequest.SetText(textParam);
        ExecuteRequest(playerNetID, fastRequest);
        return;
    }

    // yea whatever
    std::sort(missingIDs.begin(), missingIDs.end());
    missingIDs.erase(std::unique(missingIDs.begin(), missingIDs.end()), missingIDs.end());

    PendingRequest& pending = m_pendingRequests[playerNetID];
    pending.missingCount = missingIDs.size();
    pending.reqType = reqType;
    pending.params = params;
    pending.SetText(textParam);

    for(uint32 missingID : missingIDs) 
    {
        m_dbWatchers[missingID].push_back(playerNetID);

        if(m_inFlightUserIDs.find(missingID) != m_inFlightUserIDs.end())
            continue;

        m_inFlightUserIDs.insert(missingID);
        QueryRequest req = PlayerDB::GetOfflineData(missingID);
        req.callback = &UserCacheManager::OnMetadataFetched;
        req.AddExtraData(missingID);

        DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
    }
}

void UserCacheManager::OnMetadataFetched(QueryTaskResult&& result)
{
    if(result.extraData.empty())
    {
        result.Destroy();
        return;
    }

    Variant* pFetchedUser = result.GetExtraData(0);
    if(!pFetchedUser)
    {
        result.Destroy();
        return;
    }

    uint32 fetchedUserID = pFetchedUser->GetUINT();
    UserCacheManager* pCacheMgr = GetUserCacheManager();

    pCacheMgr->m_inFlightUserIDs.erase(fetchedUserID);

    if(result.status == QUERY_STATUS_OK && result.result)
    {
        for(uint32 i = 0; i < result.result->GetRowCount(); ++i)
        {
            UserMetadata meta;
            meta.userID = fetchedUserID;
           
            const string& name = result.result->GetField("Name", i).GetString();
            if(name.empty())
            {
                meta.name = result.result->GetField("GuestName", i).GetString() + "_";
                meta.name += ToString(result.result->GetField("GuestID", i).GetUINT());
            }
            else
            {
                meta.name = name;
            }

            uint32 roleID = result.result->GetField("RoleID", i).GetUINT();
            Role* pRole = GetRoleManager()->GetRole(roleID);
            if(pRole)
            {
                meta.roleID = roleID;

                if(pRole->GetNameColor() != 0)
                {
                    meta.displayName += "`";
                    meta.displayName += pRole->GetNameColor();
                }

                meta.displayName += pRole->GetPrefix() + meta.name + pRole->GetSuffix();
            }

            meta.cacheTime.Reset();
            pCacheMgr->m_cache[fetchedUserID] = std::move(meta);
        }
    }
    result.Destroy();

    auto watcherIt = pCacheMgr->m_dbWatchers.find(fetchedUserID);
    if(watcherIt != pCacheMgr->m_dbWatchers.end()) 
    {
        for(uint32 playerNetID : watcherIt->second) 
        {
            auto reqIt = pCacheMgr->m_pendingRequests.find(playerNetID);
            if(reqIt != pCacheMgr->m_pendingRequests.end()) 
            {
                reqIt->second.missingCount--; 

                if(reqIt->second.missingCount == 0) 
                {
                    pCacheMgr->ExecuteRequest(playerNetID, reqIt->second);
                    pCacheMgr->m_pendingRequests.erase(reqIt);
                }
            }
        }

        pCacheMgr->m_dbWatchers.erase(watcherIt);
    }
}

void UserCacheManager::ExecuteRequest(uint32 playerNetID, const PendingRequest& request)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(playerNetID);
    if(!pPlayer) 
        return;

    switch(request.reqType)
    {
        case CACHE_REQ_WORLD_LOCK_PUNCH:
        {
            OnPunchedLock(pPlayer, request);
            break;
        }

        case CACHE_REQ_WORLD_LOCK_DIALOG:
        {
            if(request.params.size() < 3)
                break;

            LockDialog::HandleFromCache(pPlayer, request.params[0].GetInt32(), request.params[1].GetInt32(), request.params[2].GetInt32());
            break;
        }

        case CACHE_REQ_ACHIEVEMENT_BLOCK_PUNCH:
        {
            OnPunchedAchievementBlock(pPlayer, request);
            break;
        }

        default:
            break;
    }
}

void UserCacheManager::OnPunchedLock(GamePlayer* pPlayer, const PendingRequest& request)
{
    if(!pPlayer || request.params.size() < 3)
        return;

    if(pPlayer->GetCurrentWorld() != request.params[0].GetInt32())
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(request.params[1].GetInt32(), request.params[2].GetInt32());
    if(!pTile)
        return;

    TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
    if(!pTileExtra)
        return;

    if(pTileExtra->ownerID < 1)
        return;

    UserMetadata* pUserMeta = GetMetadata(pTileExtra->ownerID);
    if(!pUserMeta)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if(pItem->type != ITEM_TYPE_LOCK)
        return;

    string notifyMsg = "`w" + pUserMeta->displayName + + "``'s `$" + pItem->name + "``.";
    if(pTileExtra->HasAccess(pPlayer->GetUserID()))
    {
        notifyMsg += " (`wAccess granted``)";
    }
    else
    {
        notifyMsg += " (`4No access``)";
    }

    pPlayer->SendOnTalkBubble(notifyMsg, true);
    pPlayer->PlaySFX("punch_locked.wav");
}

void UserCacheManager::OnPunchedAchievementBlock(GamePlayer* pPlayer, const PendingRequest& request)
{
    if(!pPlayer || request.params.size() < 3)
        return;

    if(pPlayer->GetCurrentWorld() != request.params[0].GetInt32())
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(request.params[1].GetInt32(), request.params[2].GetInt32());
    if(!pTile)
        return;

    TileExtra_Achievement* pTileExtra = pTile->GetExtra<TileExtra_Achievement>();
    if(!pTileExtra)
        return;

    UserMetadata* pUserMeta = GetMetadata(pTileExtra->ownerID);
    if(!pUserMeta)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if(pItem->type != ITEM_TYPE_ACHIEVEMENT)
        return;

    AchievementInfo* pAchievement = GetAchievementManager()->GetAchievement((eAchievement)pTileExtra->achievementID);
    if(!pAchievement)
    {
        pPlayer->SendOnTalkBubble("Invalid achievement.", true);
        return;
    }

    pPlayer->SendOnTalkBubble(pAchievement->name + " earned by " + pUserMeta->name, true);
}

void UserCacheManager::Update()
{
    if(m_lastCleanupTime.GetElapsedTime() < 300000)
        return;

    m_lastCleanupTime.Reset();

    for(auto it = m_cache.begin(); it != m_cache.end();)
    {
        if(GetPlayerManager()->GetPlayerByUserID(it->first)) 
        {
            ++it;
            continue;
        }

        if(it->second.cacheTime.GetElapsedTime() > 1200000) {
            it = m_cache.erase(it);
        } else {
            ++it;
        }
    }
}

UserCacheManager* GetUserCacheManager() { return UserCacheManager::GetInstance(); }
