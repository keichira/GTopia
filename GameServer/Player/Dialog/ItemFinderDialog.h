#pragma once
#include "Packet/PacketUtils.h"
#include "Utils/DialogPagination.h"

class GamePlayer;

class ItemFinderDialog : public DialogPagination {
public:
    explicit ItemFinderDialog(std::vector<uint16>&& itemIds) 
        : DialogPagination(30), m_fileteredItemIds(std::move(itemIds)) {}

    std::string_view GetDialogName() const override { return "item_finder"; }
    uint32 GetTotalCount() const override { return m_fileteredItemIds.size(); }

    void Render(GamePlayer* pPlayer) override;
    void OnSelectElement(GamePlayer* pPlayer, uint32 absoluteIndex) override;

private:
    std::vector<uint16> m_fileteredItemIds;
};