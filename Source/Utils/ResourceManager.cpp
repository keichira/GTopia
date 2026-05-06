#include "ResourceManager.h"
#include "../IO/File.h"
#include "../Proton/ProtonUtils.h"
#include "../Utils/ZLibUtils.h"
#include "../Item/ItemInfoManager.h"
#include "../IO/Log.h"

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
    Kill();
}

BLImage* ResourceManager::LoadTileSheet(const string& path)
{
    if(path.empty()) {
        return nullptr;
    }

    string fullPath = m_path + "/" + path;

    BLImage* pExistImg = IsTileSheetExists(path);
    if(pExistImg) {
        return pExistImg;
    }

    if(GetFileExtension(path) == "rttex") {
        File file;
        if(!file.Open(fullPath)) {
            return nullptr;
        }
    
        uint32 fileSize = file.GetSize();
        uint8* pData = new uint8[fileSize];
    
        if(file.Read(pData, fileSize) != fileSize) {
            SAFE_DELETE_ARRAY(pData);
            file.Close();
            return nullptr;
        }
    
        MemoryBuffer memBuffer(pData, fileSize);
    
        Proton::rtpack_header rtPackHeader;
        memBuffer.Read(rtPackHeader);
    
        Proton::rttex_header rtTexHeader;
    
        if(rtPackHeader.compressionType == 1) {
            uint8* pDecomp = zLibInflateToMemory(pData + memBuffer.GetOffset(), rtPackHeader.compressedSize, rtPackHeader.decompressedSize);
            if(!pDecomp) {
                SAFE_DELETE_ARRAY(pData);
                file.Close();
                return nullptr;
            }
    
            SAFE_DELETE_ARRAY(pData);
            pData = pDecomp;
            memBuffer = MemoryBuffer(pDecomp, rtPackHeader.decompressedSize);
        }
    
        memBuffer.Read(rtTexHeader);
    
        BLImage* pImg = new BLImage(rtTexHeader.width, rtTexHeader.height, BL_FORMAT_PRGB32);
        BLImageData imgData;
        pImg->get_data(&imgData);
    
        int32 format = rtTexHeader.format;
    
        Proton::rttex_mip_header texMip;
        memBuffer.Read(texMip);
    
        uint8* pTextureData = new uint8[texMip.dataSize];
        memBuffer.ReadRaw(pTextureData, texMip.dataSize);
        
        FlipVerticalWithPremultiply(pTextureData, rtTexHeader.width, rtTexHeader.height, rtTexHeader.bUsesAlpha);
        memcpy(imgData.pixel_data, pTextureData, rtTexHeader.width * rtTexHeader.height * 4);
    
        SAFE_DELETE_ARRAY(pTextureData);
        SAFE_DELETE_ARRAY(pData);
        file.Close();
    
        m_tileSheets[path] = pImg;
        return pImg;
    }

    BLImage* pImg = new BLImage();
    if(pImg->read_from_file(fullPath.c_str()) != BL_SUCCESS) {
        SAFE_DELETE(pImg);
        return nullptr;
    }

    m_tileSheets[path] = pImg;
    return pImg;
}

BLImage* ResourceManager::IsTileSheetExists(const string& path)
{
    auto it = m_tileSheets.find(path);
    if(it != m_tileSheets.end()) {
        return it->second;
    }

    return nullptr;
}

BLImage* ResourceManager::GetItemTileSheet(uint32 itemID)
{
    auto it = m_itemTileSheets.find(itemID);
    if(it != m_itemTileSheets.end()) {
        return it->second;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    string path = pItem ? ("game/" + pItem->textureFile) : "game/tiles_page1.rttex";

    BLImage* pImg = LoadTileSheet(path);
    if(!pImg && path != "game/tiles_page1.rttex") {
        pImg = LoadTileSheet("game/tiles_page1.rttex");
    }

    if(pImg) {
        m_itemTileSheets[itemID] = pImg;
    }

    return pImg;
}

void ResourceManager::Kill()
{
    for(auto& [_, pImg] : m_tileSheets) {
        SAFE_DELETE(pImg);
    }

    for(auto& [_, pFont] : m_fonts) {
        SAFE_DELETE(pFont);
    }

    m_tileSheets.clear();
    m_itemTileSheets.clear();
    m_fonts.clear();
}

BLImage* ResourceManager::GetTileSheet(const string& tileSheet)
{
    auto it = m_tileSheets.find(tileSheet);
    if(it != m_tileSheets.end()) {
        return it->second;
    }

    BLImage* pImg = LoadTileSheet(tileSheet);
    if(!pImg) {
        pImg = GetTileSheet("game/tiles_page1.rttex");
        if(!pImg) {
            return nullptr;
        }

        return pImg;
    }

    return pImg;
}

BLFont* ResourceManager::LoadFont(uint16 fontID, const string& path)
{
    if(path.empty()) {
        return nullptr;
    }

    string fullPath = m_path + "/" + path;

    BLFontFace fontFace;
    if(fontFace.create_from_file(fullPath.c_str()) != BL_SUCCESS) {
        return nullptr;
    }

    BLFont* pFont = new BLFont();
    pFont->create_from_face(fontFace, 16.0f);

    m_fonts[fontID] = pFont;
    return pFont;
}

BLFont* ResourceManager::GetFont(uint16 fontID)
{
    auto it = m_fonts.find(fontID);
    if(it != m_fonts.end()) {
        return it->second;
    }

    return nullptr;
}

void ResourceManager::FlipVerticalWithPremultiply(uint8* pPixelData, int32 width, int32 height, bool premultiply)
{
    uint32 rowBytes = width * 4; // uhh

   for (int32 y = 0; y < height / 2; ++y)
   {
       uint8* top = pPixelData + y * rowBytes;
       uint8* bottom = pPixelData + (height - 1 - y) * rowBytes;
   
       for (int32 x = 0; x < width; ++x)
       {
           uint8* t = top + x * 4;
           uint8* b = bottom + x * 4;
   
           for (int i = 0; i < 4; ++i) {
            std::swap(t[i], b[i]);
           }
   
           std::swap(t[0], t[2]);
           std::swap(b[0], b[2]);
       }
   }
}

ResourceManager* GetResourceManager() { return ResourceManager::GetInstance(); }