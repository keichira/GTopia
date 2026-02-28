#include "Precompiled.h"
#include "Item/ItemInfoManager.h"
#include "Network/NetHTTP.h"
#include "IO/File.h"
#include "IO/Log.h"
#include "Utils/StringUtils.h"
#include "Utils/Timer.h"
#include <nlohmann/json.hpp>

/**
 * 
 * Some items wiki data might broken beacuse the wiki page is broken for some items
 * 
 */

struct ItemWikiData
{
    string description;
    string element;
    string seed1;
    string seed2;
};

bool ParseItemWiki(const string& data, ItemWikiData& out)
{
    auto TrimBraces = [&](const string& str) -> string
    {
        int32 pos = str.find("}}");
        if (pos != -1) {
            return str.substr(0, pos);
        }
        return "";
    };

    auto lines = Split(data, '\n');
    for(auto& line : lines) {
        if(line.empty()) {
            continue;
        }

        auto args = Split(line, '|');
        if(args.empty()) {
            continue;
        }

        if(args[0] == "{{Item") {
            if(args.size() > 2) {
                out.description = args[1];
                out.element = TrimBraces(args[2]);
            }
            else {
                out.description = TrimBraces(args[1]);
            }
        }

        if(args[0] == "{{RecipeSplice") {
            if(args.size() > 3) {
                out.seed1 = args[1];
                out.seed2 = TrimBraces(args[2]);
            }
            else {
                out.seed1 = args[1];
                out.seed2 = TrimBraces(args[2]);
            }
        }
    }

    return true;
}

bool ParseWiki(const string& data, ItemWikiData& out) 
{
    try
    {
        nlohmann::json json = nlohmann::json::parse(data);
        if(json.contains("query") && json["query"].contains("pages")) {
            for(auto& [pageID, pageData] : json["query"]["pages"].items()) {
                if(pageData.contains("revisions")) {
                    auto data = pageData["revisions"][0]["*"].get<string>();
    
                    return ParseItemWiki(data, out);
                }
            }
        }
    }
    catch(const std::exception& e)
    {
        return false;
    }

    return false;
}

int main(int argc, char const* argv[])
{
    ItemInfoManager* pItemMgr = GetItemInfoManager();
    if(!pItemMgr->LoadByItemsDat(GetProgramPath() + "/items.dat")) {
        LOGGER_LOG_ERROR("Failed to load items.dat");
        return 0;
    }

    uint32 itemCount = pItemMgr->GetItemCount();
    uint32 startFrom = 0;
    if(argc > 1) {
        itemCount = ToUInt(argv[1]);
    }
    else if(argc > 2) {
        itemCount = ToUInt(argv[1]);
        startFrom = ToUInt(argv[2]);
    }

    File file;
    FileMode fMode = startFrom == 0 ? FILE_MODE_WRITE : FILE_MODE_APPEND;
    
    if(!file.Open(GetProgramPath() + "/items_generated.txt", fMode)) {
        LOGGER_LOG_ERROR("Failed to open file to write");
        return 0;
    }

    NetHTTP http;
    http.Init("https://growtopia.fandom.com");

    uint16 lastSeed1 = 0;
    uint16 lastSeed2 = 0;

    uint64 startTime = Time::GetSystemTime();

    for(uint32 i = startFrom; i < itemCount; ++i) {
        ItemInfo* pItem = pItemMgr->GetItemByID(i);
        if(!pItem) {
            LOGGER_LOG_ERROR("Item %d not found skipping...?", i);
            continue;
        }

        LOGGER_LOG_INFO("Processing item %d %s (%d/%d)", pItem->id, pItem->name.c_str(), i + 1, itemCount);

        string itemStr;
        if(pItem->name.find("null_item") != -1) {
            uint32 startID = pItem->id;
            uint32 endID = pItem->id + 1;

            while(i + 1 < itemCount) {
                ItemInfo* pNextItem = pItemMgr->GetItemByID(i + 1);
                if(!pNextItem || pNextItem->name.find("null_item") == -1) {
                    break;
                }

                ++i;
                endID = pNextItem->id;
            }

            itemStr += "make_null|" + ToString(startID) + "|" + ToString(endID) + "|\n\n";
            file.Write(itemStr.data(), itemStr.size());
            continue;
        }
        else if(pItem->type == ITEM_TYPE_CLOTHES) {
            itemStr += "add_cloth|";
            itemStr += ToString(pItem->id) + "|";
            itemStr += pItem->name + "|";
            itemStr += ItemMaterialToStr(pItem->material) + "|";
            itemStr += pItem->textureFile + "|";
            itemStr += ToString(pItem->textureX) + "," + ToString(pItem->textureY) + "|";
            itemStr += ItemVisualEffectToStr(pItem->visualEffect) + "|";
            itemStr += ItemStorageTypeToStr(pItem->storage) + "|";
            itemStr += ItemBodyPartToStr(pItem->bodyPart) + "|\n";
        }
        else if(pItem->type == ITEM_TYPE_SEED) {
            itemStr += "set_seed|";
            itemStr += ToString(lastSeed1) + "|";
            itemStr += ToString(lastSeed2) + "|";
            itemStr += ToString(pItem->seedBgColor.r) + "," + ToString(pItem->seedBgColor.g) + "," + ToString(pItem->seedBgColor.b) + "," + ToString(pItem->seedBgColor.a) + "|";
            itemStr += ToString(pItem->seedFgColor.r) + "," + ToString(pItem->seedFgColor.g) + "," + ToString(pItem->seedFgColor.b) + "," + ToString(pItem->seedFgColor.a) + "|\n";
            itemStr += "\n";
            
            file.Write(itemStr.data(), itemStr.size());
            continue;
        }
        else {
            itemStr += "add_item|";
            itemStr += ToString(pItem->id) + "|";
            itemStr += pItem->name + "|";
            itemStr += ItemTypeToStr(pItem->type) + "|";
            itemStr += ItemMaterialToStr(pItem->material) + "|";
            itemStr += pItem->textureFile + "|";
            itemStr += ToString(pItem->textureX) + "," + ToString(pItem->textureY) + "|";
            itemStr += ItemVisualEffectToStr(pItem->visualEffect) + "|";
            itemStr += ItemStorageTypeToStr(pItem->storage) + "|";
            itemStr += ItemCollisionTypeToStr(pItem->collisionType) + "|";
            itemStr += ToString(pItem->hp/6) + "|";
            itemStr += ToString(pItem->restoreTime) + "|\n";
        }

        string httpPath = "/api.php?action=query&prop=revisions&titles=" + pItem->name + "&rvprop=content&format=json";
        if(pItem->id != ITEM_ID_GEMS && http.Get(httpPath)) {
            if(http.GetStatus() == 200 && !http.GetBody().empty()) {
                ItemWikiData data;
                if(!ParseWiki(http.GetBody(), data)) {
                    LOGGER_LOG_ERROR("Failed to parse wiki data for %d", pItem->id);
                }
                else {
                    if(!data.description.empty()) {
                        itemStr += "description|" + data.description + "|\n";
                    }
    
                    if(!data.element.empty()) {
                        itemStr += "set_element|" + ToUpper(data.element) + "|\n";
                    }
    
                    if(!data.seed1.empty() && !data.seed2.empty()) {
                        ItemInfo* pSeed1 = pItemMgr->GetItemByName(data.seed1);
                        ItemInfo* pSeed2 = pItemMgr->GetItemByName(data.seed2);
    
                        if(!pSeed1 || !pSeed2) {
                            LOGGER_LOG_ERROR("Seed not found for %d", pItem->id);
                            lastSeed1 = lastSeed2 = 0;
                        }
                        else {
                            lastSeed1 = pSeed1->id + 1;
                            lastSeed2 = pSeed2->id + 1;
                        }
                    }
                    else {
                        lastSeed1 = lastSeed2 = 0;
                    }
    
                    SleepMS(10);
                }
            }
            else {
                LOGGER_LOG_ERROR("HTTP Get failed for %d bodySize %d code %d", pItem->id, http.GetBody().size(), http.GetStatus());
                lastSeed1 = lastSeed2 = 0;
            }
        }
        else {
            LOGGER_LOG_ERROR("HTTP GET failed for %d error %d", pItem->id, http.GetError());
            lastSeed1 = lastSeed2 = 0;
        }

        if(pItem->flags != 0) {
            string itemFlagStr;

            for(uint8 i = 0; i < 16; ++i) {
                if(pItem->flags & (1 << i)) {
                    if(!itemFlagStr.empty()) {
                        itemFlagStr += ",";
                    }

                    itemFlagStr += ItemFlagToStr(1 << i);
                }
            }

            itemStr += "set_flags|" + itemFlagStr + "|\n";
        }

        if(!pItem->extraString.empty() || pItem->animMS != 200) {
            itemStr += "set_extra|" + pItem->extraString + "|" + ToString(pItem->animMS) + "|\n";
        }

        file.Write(itemStr.data(), itemStr.size());
    }

    file.Close();

    LOGGER_LOG_INFO("Finished! took %dms (with HTTP sleeps)", Time::GetSystemTime() - startTime);
}
