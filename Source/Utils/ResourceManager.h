#pragma once

#include "../Precompiled.h"
#include <blend2d/blend2d.h>

class ResourceManager {
public:
    ResourceManager();
    ~ResourceManager();

public:
    static ResourceManager* GetInstance()
    {
        static ResourceManager instance;
        return &instance;
    }

public:
    void SetResourcePath(const string& path) { m_path = path; };

    BLImage* LoadTileSheet(const string& path);
    BLImage* IsTileSheetExists(const string& path);
    BLImage* GetItemTileSheet(uint32 itemID);
    BLImage* GetTileSheet(const string& tileSheet);

    BLFont* LoadFont(uint16 fontID, const string& path);
    BLFont* GetFont(uint16 fontID);

    void Kill();

private:
    void FlipVerticalWithPremultiply(uint8* pPixelData, int32 width, int32 height, bool premultiply);

private:
    string m_path;
    std::unordered_map<string, BLImage*> m_tileSheets;
    std::unordered_map<uint16, BLImage*> m_itemTileSheets;
    std::unordered_map<uint16, BLFont*> m_fonts;
};

ResourceManager* GetResourceManager();