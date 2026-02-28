#pragma once

#include "../Precompiled.h"
#include "../Math/Vector2.h"
#include "../Memory/MemoryBuffer.h"
#include "TileExtra.h"

enum eTileFlags 
{
    TILE_FLAG_HAS_EXTRA_DATA = 1 << 0,
    TILE_FLAG_HAS_PARENT = 1 << 1,
    TILE_FLAG_WAS_SPLICED = 1 << 2,
    TILE_FLAG_WILL_SPAWN_SEEDS_TOO = 1 << 3,
    TILE_FLAG_IS_SEEDLING = 1 << 4,
    TILE_FLAG_FLIPPED_X = 1 << 5,
    TILE_FLAG_IS_ON = 1 << 6,
    TILE_FLAG_IS_OPEN_TO_PUBLIC = 1 << 7,
    TILE_FLAG_BG_IS_ON = 1 << 8,
    TILE_FLAG_FG_ALT_MODE = 1 << 9,
    TILE_FLAG_IS_WET = 1 << 10,
    TILE_FLAG_GLUED = 1 << 11,
    TILE_FLAG_ON_FIRE = 1 << 12,
    TILE_FLAG_PAINTED_RED = 1 << 13,
    TILE_FLAG_PAINTED_GREEN = 1 << 14,
    TILE_FLAG_PAINTED_BLUE = 1 << 15
};

class TileInfo {
public:
    TileInfo();
    ~TileInfo();

public:
    void Serialize(MemoryBuffer& memBuffer, bool write, bool database);

    void SetFG(uint16 itemID);
    void SetBG(uint16 itemID);

    void SetPos(Vector2Int pos) { m_pos = pos; }
    Vector2Int GetPos() const { return m_pos; }

    void SetFlag(uint16 flag) { m_flags |= flag; }
    void RemoveFlag(uint16 flag) { m_flags &= ~flag; }
    bool HasFlag(uint16 flag) { return m_flags & flag; };
    void ToggleFlag(uint16 flag) { m_flags ^= flag; }

private:
    uint16 m_fg;
    uint16 m_bg;
    uint16 m_parent;
    uint16 m_flags;
    Vector2Int m_pos;

    TileExtra* m_pExtraData;
};