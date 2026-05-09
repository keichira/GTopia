#pragma once

#include "../Precompiled.h"
#include "../Math/Vector2.h"
#include "../Memory/MemoryBuffer.h"
#include "../Packet/GamePacket.h"
#include "../Math/Rect.h"

enum eObjectFlag
{
    OBJECT_FLAG_DIRTY = 1 << 1,
    OBJECT_FLAG_NO_STACK = 1 << 2
};

struct WorldObject
{
    uint16 itemID = 0;
    Vector2Float pos;
    uint8 count = 0;
    uint8 flags = 0;
    uint32 objectID = 0;

    void SetFlag(uint16 flag) { flags |= flag; }
    void RemoveFlag(uint16 flag) { flags &= ~flag; }
    bool HasFlag(uint16 flag) { return flags & flag; };

    void Serialize(MemoryBuffer& memBuffer, bool write);
};

class WorldObjectManager {
public:
    WorldObjectManager();
    ~WorldObjectManager();

public:
    void Serialize(MemoryBuffer& memBuffer, bool write, bool database);
    uint32 GetMemEstimate();
    void AddItem(uint16 itemID, uint8 count, Vector2Float pos, uint8 flags = 0);
    void ModifyItem(uint32 objectID, uint16 itemID, uint8 count, Vector2Float pos, uint8 flags);
    void DeleteByID(uint32 objectID);
    void HandleObjectPackets(GameUpdatePacket* pGamePacket);

    std::vector<WorldObject*> GetObjectsInRect(RectFloat& rect);
    std::vector<WorldObject*> GetObjectsInRectByItemID(RectFloat& rect, uint16 itemID);
    WorldObject* GetObjectByID(uint32 objectID);

private:
    std::vector<WorldObject> m_objects;
    int32 m_lastObjectID;
};