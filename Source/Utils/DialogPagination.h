#pragma once

#include "../Precompiled.h"
#include "../Packet/PacketUtils.h"

class GamePlayer;

class DialogPagination {
public:
    explicit DialogPagination(uint32 pageSize = 30) : m_pageSize(pageSize) {}
    virtual ~DialogPagination() = default;

    virtual void Render(GamePlayer* pPlayer) = 0;
    virtual std::string_view GetDialogName() const = 0;
    virtual void OnSelectElement(GamePlayer* pPlayer, uint32 absoluteIndex) = 0;
    virtual uint32 GetTotalCount() const = 0;

public:
    uint32 GetMaxPageCount() const { return (GetTotalCount() + m_pageSize - 1) / m_pageSize; }

    bool ProcessButton(GamePlayer* pPlayer, TextPacketField* pClickedButton) 
    {
        if(!pClickedButton)
            return false;

        if(pClickedButton->keySize == 0)
            return false;

        if(pClickedButton->GetStringView() == "prev") 
        {
            if(m_page > 0) 
            {
                --m_page;
                Render(pPlayer);
            }
            return true;
        }

        if(pClickedButton->GetStringView() == "next") 
        {
            if(m_page + 1 < GetMaxPageCount())
            {
                ++m_page;
                Render(pPlayer);
            }
            return true;
        }

        uint32 index = 0;
        if(pClickedButton->GetUInt(index) != TO_INT_SUCCESS)
            return false;

        uint32 absoluteIdx = (m_page * m_pageSize) + index;
        if(absoluteIdx < GetTotalCount())
        {
            OnSelectElement(pPlayer, absoluteIdx);
        }

        return true;
    }

protected:
    uint32 m_page = 0;
    uint32 m_pageSize = 30;
};