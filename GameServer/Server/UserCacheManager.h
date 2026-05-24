#pragma once

#include "Precompiled.h"
#include <unordered_set>
#include <array>
#include "Database/Table/PlayerDBTable.h"
#include "Utils/Timer.h"

enum eCacheRequestType : uint8
{
    CACHE_REQ_NONE,
    CACHE_REQ_WORLD_LOCK_PUNCH
};

union CacheParam
{
    int32 i32;
    uint32 u32;
    float f32;
    bool b;

    CacheParam() : i32(0) {}
    CacheParam(int32 val) : i32(val) {}
    CacheParam(uint32 val) : u32(val) {}
    CacheParam(float val) : f32(val) {}
    CacheParam(bool val) : b(val) {}

    int32 GetInt32() const { return i32; }
    uint32 GetUInt32() const { return u32; }
    float GetFloat() const { return f32; }
    bool GetBool() const { return b; }
};

struct UserMetadata
{
    uint32 userID = 0;
    string name;
    string displayName;
    uint32 roleID = 0;
    Timer cacheTime;
};

class UserCacheManager {
public:
    UserCacheManager();
    ~UserCacheManager();

public:
    static UserCacheManager* GetInstance()
    {
        static UserCacheManager instance;
        return &instance;
    }

public:
    void Init(uint32 maxExpectedPlayers);

    UserMetadata* GetMetadata(uint32 userID);
    bool IsCached(uint32 userID);
    void FetchMetadata(uint32 playerNetID,
                       eCacheRequestType reqType,
                       const std::vector<uint32>& userIDs,  
                       std::array<CacheParam, 5> params = {}, 
                       const char* textParam = nullptr);
    void Update();    
    static void OnMetadataFetched(QueryTaskResult&& result);

private:
    struct PendingRequest
    {
        std::vector<uint32> missingUserIDs;
        uint32 missingCount = 0;
        eCacheRequestType reqType = CACHE_REQ_NONE;
        std::array<CacheParam, 5> params;
        char textParam[32] = { 0 }; 

        void SetText(const char* src)
        {
            if(!src) 
            {
                textParam[0] = '\0';
                return;
            }

            usize len = strnlen(src, 32);
            if(len >= 32)
            {
                textParam[0] = '\0';
                return;
            }
            
            memcpy(textParam, src, len);
            textParam[len] = '\0';
        }
    };

    void ExecuteRequest(uint32 playerNetID, const PendingRequest& request);

    std::unordered_map<uint32, UserMetadata> m_cache;
    std::unordered_map<uint32, PendingRequest> m_pendingRequests;
    std::unordered_set<uint32> m_inFlightUserIDs; 
    std::unordered_map<uint32, std::vector<uint32>> m_dbWatchers;

    Timer m_lastCleanupTime;
};

UserCacheManager* GetUserCacheManager();