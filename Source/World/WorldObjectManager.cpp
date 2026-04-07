#include "WorldObjectManager.h"

void WorldObject::Serialize(MemoryBuffer& memBuffer, bool write)
{
    memBuffer.ReadWrite(itemID, write);
    memBuffer.ReadWrite(pos, write);
    memBuffer.ReadWrite(count, write);
    memBuffer.ReadWrite(flags, write);
    memBuffer.ReadWrite(objectID, write);
}

WorldObjectManager::WorldObjectManager()
: m_lastObjectID(0)
{
}

WorldObjectManager::~WorldObjectManager()
{
}

void WorldObjectManager::Serialize(MemoryBuffer& memBuffer, bool write)
{
    int32 objectCount = m_objects.size();
    memBuffer.ReadWrite(objectCount, write);
    memBuffer.ReadWrite(m_lastObjectID, write);

    if(!write) {
        m_objects.resize(objectCount);
    }

    for(auto& obj : m_objects) {
        obj.Serialize(memBuffer, write);
    }
}

void WorldObjectManager::AddItem(uint16 itemID, uint8 count, Vector2Float pos, uint8 flags)
{
    WorldObject obj;
    obj.itemID = itemID;
    obj.pos = pos;
    obj.count = count;
    obj.flags = flags;
    obj.objectID = m_lastObjectID++;

    m_objects.push_back(std::move(obj));
}

void WorldObjectManager::ModifyItem(uint32 objectID, uint16 itemID, uint8 count, Vector2Float pos, uint8 flags)
{
    WorldObject* pObject = GetObjectByID(objectID);
    if(!pObject) {
        return;
    }

    pObject->itemID = itemID;
    pObject->count = count;
    pObject->pos = pos;
    pObject->flags = flags;
}

void WorldObjectManager::DeleteByID(uint32 objectID)
{
    for(auto& object : m_objects) {
        if(object.objectID == objectID) {
            if(&object != &m_objects.back()) {
                object = std::move(m_objects.back());
            }
            m_objects.pop_back();
            return;
        }
    }
}

void WorldObjectManager::HandleObjectPackets(GameUpdatePacket* pGamePacket)
{
    if(!pGamePacket) {
        return;
    }

    if(pGamePacket->worldObjectType == -3) { // modify
        ModifyItem(pGamePacket->worldObjectID, pGamePacket->itemID, pGamePacket->worldObjectCount, Vector2Float(pGamePacket->posX, pGamePacket->posY), pGamePacket->worldObjectFlags);
    }
    else if(pGamePacket->worldObjectType == -1) { // add
        AddItem(pGamePacket->itemID, (uint8)pGamePacket->worldObjectCount, Vector2Float(pGamePacket->posX, pGamePacket->posY), pGamePacket->worldObjectFlags);
    }
    else { // remove
        DeleteByID(pGamePacket->worldObjectID);
    }
}

std::vector<WorldObject*> WorldObjectManager::GetObjectsInRect(RectFloat& rect)
{
    std::vector<WorldObject*> objsInRect;

    for(auto& obj : m_objects) {
        if(rect.IsInside(obj.pos)) {
            objsInRect.push_back(&obj);
        }
    }

    return objsInRect;
}

std::vector<WorldObject*> WorldObjectManager::GetObjectsInRectByItemID(RectFloat& rect, uint16 itemID)
{
    std::vector<WorldObject*> objsInRect;

    for(auto& obj : m_objects) {
        if(rect.IsInside(obj.pos) && obj.itemID == itemID) {
            objsInRect.push_back(&obj);
        }
    }

    return objsInRect;
}

WorldObject* WorldObjectManager::GetObjectByID(uint32 objectID)
{
    for(auto& obj : m_objects) {
        if(obj.objectID == objectID) {
            return &obj;
        }
    }

    return nullptr;
}
