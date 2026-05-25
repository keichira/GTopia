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

void WorldObjectManager::Serialize(MemoryBuffer& memBuffer, bool write, bool database)
{
    uint32 objectCount = m_objects.size();
    memBuffer.ReadWrite(objectCount, write);
    memBuffer.ReadWrite(m_lastObjectID, write);

    if(!write) 
    {
        m_objects.resize(objectCount);
    }

    if(database)
    {
        m_lastObjectID = 0;
    }

    for(auto& obj : m_objects) // because of padding need to serialize like this way
    {
        obj.Serialize(memBuffer, write);

        if(database)
        {
            obj.objectID = ++m_lastObjectID;
        }
    }
}

uint32 WorldObjectManager::GetMemEstimate()
{
    return sizeof(uint32) + sizeof(m_lastObjectID) + m_objects.size() * sizeof(WorldObject);
}

void WorldObjectManager::AddItem(uint16 itemID, uint8 count, Vector2Float pos, uint8 flags)
{
    WorldObject obj;
    obj.itemID = itemID;
    obj.pos = pos;
    obj.count = count;
    obj.flags = flags;
    obj.objectID = ++m_lastObjectID;

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
    if(!pGamePacket)
        return;

    if(pGamePacket->field_4 == -3) 
    { // modify
        ModifyItem(
            pGamePacket->field_5, 
            pGamePacket->field_7, 
            pGamePacket->field_6, 
            Vector2Float(pGamePacket->field_8.x, pGamePacket->field_8.y), 
            pGamePacket->field_1
        );
    }
    else if(pGamePacket->field_4 == -1) 
    { // add
        AddItem(
            pGamePacket->field_7,
            (uint8)pGamePacket->field_6, 
            Vector2Float(pGamePacket->field_8.x, pGamePacket->field_8.y), 
            pGamePacket->field_1
        );
    }
    else { // remove
        DeleteByID(pGamePacket->field_7);
    }
}

std::vector<WorldObject*> WorldObjectManager::GetObjectsInRect(const RectFloat& rect)
{
    std::vector<WorldObject*> objsInRect;

    for(auto& obj : m_objects) {
        if(rect.IsInside(obj.GetCenterPos())) {
            objsInRect.push_back(&obj);
        }
    }

    return objsInRect;
}

std::vector<WorldObject*> WorldObjectManager::GetObjectsInRectByItemID(const RectFloat& rect, uint16 itemID)
{
    std::vector<WorldObject*> objsInRect;

    for(auto& obj : m_objects) {
        if(rect.IsInside(obj.GetCenterPos()) && obj.itemID == itemID) {
            objsInRect.push_back(&obj);
        }
    }

    return objsInRect;
}

uint32 WorldObjectManager::GetCounfOfObjestsInRect(const RectFloat& rect)
{
    uint32 count = 0;
    for(auto& obj : m_objects)
    {
        if(rect.IsInside(obj.pos))
        {
            count++;
        }
    }

    return count;
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
