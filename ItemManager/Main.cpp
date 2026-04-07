#include "Precompiled.h"
#include "Item/ItemInfoManager.h"
#include "Network/NetHTTP.h"
#include "IO/File.h"
#include "IO/Log.h"
#include "Utils/StringUtils.h"
#include "Utils/Timer.h"
#include <nlohmann/json.hpp>
#include <regex>

/**
 * 
 * Some items wiki data might broken beacuse the wiki page is broken for some items
 * 
 */

struct ItemWikiData
{
    string description;
    string element = "NONE";
    string seed1;
    string seed2;

    string combine1;
    string combine1Amount;
    string combine2;
    string combine2Amount;
    string combine3;
    string combine3Amount;
    string combineResultAmount;
};

string TrimBraces(const string& str) {
    usize pos = str.find("}}");
    if(pos != string::npos) {
        return str.substr(0, pos);
    }

    pos = str.find('}');
    if(pos != string::npos) {
        return str.substr(0, pos);
    }
    return str;
}

std::vector<string> ExtractTemplates(const string& data) {
    std::vector<string> templates;

    std::regex rgx(R"(\{\{.*?\}\})");

    for(auto i = std::sregex_iterator(data.begin(), data.end(), rgx); i != std::sregex_iterator(); ++i) {
        templates.push_back(i->str());
    }
    return templates;
}

void ParseItemWiki(const string& data, ItemWikiData& out) {
    auto templates = ExtractTemplates(data);

    for(auto& tpl : templates) {
        auto args = Split(tpl, '|');
        if(args.empty()) {
            continue;
        }

        string type = TrimBraces(args[0]);

        if(type == "{{Item") {
            if(args.size() >= 2) {
                out.description = TrimBraces(args[1]);
            }
        
            if(args.size() >= 3) {
                out.element = TrimBraces(args[2]);
            }
        }
        else if(type == "{{RecipeSplice") {
            if(args.size() >= 2) {
                out.seed1 = args[1];
            }
            if(args.size() >= 3) {
                out.seed2 = TrimBraces(args[2]);
            }
        }
        else if(type == "{{RecipeCombine") {
            if(args.size() < 6) {
                continue;
            }

            out.combine1 = args[1];
            out.combine1Amount = args[2];
            out.combine2 = args[3];
            out.combine2Amount = args[4];
            out.combine3 = args[5];
        
            if(args.size() >= 7) {
                out.combine3Amount = TrimBraces(args[6]);
            }
        
            if(args.size() >= 8) {
                out.combineResultAmount = TrimBraces(args[7]);
            } else {
                out.combineResultAmount = "1";
            }
        }
    }
}

void FetchWikiAndWrite(uint32 startFromID, uint32 serializeUntil = 0)
{
    File file;
    FileMode fMode = startFromID == 0 ? FILE_MODE_WRITE : FILE_MODE_APPEND;
    if(!file.Open(GetProgramPath() + "/wiki_data_generated.txt", fMode)) {
        LOGGER_LOG_ERROR("Failed to open wiki_data_generated.txt");
        return;
    }

    ItemInfoManager* pItemMgr = GetItemInfoManager();
    if(!pItemMgr->LoadByItemsDat(GetProgramPath() + "/items.dat")) {
        LOGGER_LOG_ERROR("Failed to load items.dat");
        return;
    }

    if(startFromID > pItemMgr->GetItemCount()) {
        return;
    }

    if(serializeUntil == 0 || serializeUntil > pItemMgr->GetItemCount()) {
        serializeUntil = pItemMgr->GetItemCount();
    }

    NetHTTP http;
    http.Init("https://growtopia.fandom.com");

    LOGGER_LOG_INFO("Started getting item names for batch");
    std::vector<std::string> batchItemNames;
    std::string currentBatchNames;
    int32 currItemBatchSize = 0;
    
    for(int32 i = startFromID; i < serializeUntil + startFromID; ++i) {
        ItemInfo* pItem = pItemMgr->GetItemByID(i);
        if (!pItem) {
            LOGGER_LOG_ERROR("Not adding item %d for batching its NULL", i);
            continue;
        }

        if(pItem->id % 2 != 0) {
            continue;
        }
    
        if(currItemBatchSize > 0) {
            currentBatchNames += "|";
        }
        currentBatchNames += pItem->name;
        ++currItemBatchSize;
    
        if (currItemBatchSize == 49) {
            batchItemNames.push_back(currentBatchNames);
            currentBatchNames.clear();
            currItemBatchSize = 0;
        }
    }

    if(!currentBatchNames.empty()) {
        batchItemNames.push_back(currentBatchNames);
    }

    LOGGER_LOG_INFO("Started fetching from wiki");
    for(int32 i = 0; i < batchItemNames.size(); ++i) {
        if(!http.Get("/api.php?action=query&prop=revisions&titles=" + batchItemNames[i] + "&rvprop=content&format=json")) {
            LOGGER_LOG_WARN("Failed to GET on %s", batchItemNames[i].c_str());
            continue;
        }

        if(http.GetStatus() != 200) {
            LOGGER_LOG_WARN("Got status code %d on %s", http.GetStatus(), batchItemNames[i].c_str());
            continue;
        }

        string body = http.GetBody();
        if(body.empty()) {
            LOGGER_LOG_WARN("Body is empty! %s", batchItemNames[i].c_str());
            continue;
        }

        try {
            nlohmann::json json = nlohmann::json::parse(body);
            if(json.contains("query") && json["query"].contains("pages")) {
                string itemName;
    
                for(auto& [pageID, pageData] : json["query"]["pages"].items()) {
                    if(pageData.contains("title")) {
                        itemName = pageData["title"].get<string>();
                    }
    
                    if(pageData.contains("revisions")) {
                        auto data = pageData["revisions"][0]["*"].get<string>();
    
                        if(itemName.empty()) {
                            continue;
                        }
    
                        ItemWikiData wikiData;
                        ParseItemWiki(data, wikiData);
                        
                        ItemInfo* pItem = pItemMgr->GetItemByName(itemName);
                        if(!pItem) {
                            LOGGER_LOG_WARN("Skipping %s is not exist??", itemName.c_str());
                            continue;
                        }
    
                        string writeData = "#" + itemName + "\n";
                        writeData += "set_wiki|" + ToString(pItem->id) + "|";
                        if(wikiData.seed1.empty() && wikiData.seed2.empty()) {
                            writeData += "0|0|";
                        }
                        else {
                            ItemInfo* pItemSeed1 = pItemMgr->GetItemByName(wikiData.seed1);
                            ItemInfo* pItemSeed2 = pItemMgr->GetItemByName(wikiData.seed2);

                            writeData += pItemSeed1 ? ToString(pItemSeed1->id + 1) + "|" : "0|";
                            writeData += pItemSeed2 ? ToString(pItemSeed2->id + 1) + "|" : "0|";
                        }
                        writeData += ToUpper(wikiData.element) + "|";
                        writeData += wikiData.description + "|";

                        if(!wikiData.combine1.empty() && !wikiData.combine2.empty() && !wikiData.combine3.empty()) {
                            writeData += "\n";
                            writeData += "set_combine|" + ToString(pItem->id) + "|";

                            ItemInfo* pItemCombine1 = pItemMgr->GetItemByName(wikiData.combine1);
                            ItemInfo* pItemCombine2 = pItemMgr->GetItemByName(wikiData.combine2);
                            ItemInfo* pItemCombine3 = pItemMgr->GetItemByName(wikiData.combine3);

                            writeData += pItemCombine1 ? ToString(pItemCombine1->id) + "|" : "0|";
                            writeData += wikiData.combine1Amount + "|";

                            writeData += pItemCombine2 ? ToString(pItemCombine2->id) + "|" : "0|";
                            writeData += wikiData.combine2Amount + "|";

                            writeData += pItemCombine3 ? ToString(pItemCombine3->id) + "|" : "0|";
                            writeData += wikiData.combine3Amount + "|";
                        }
                        else {
                            //LOGGER_LOG_WARN("Got empty combine on %d %s skipping", pItem->id, itemName.c_str());
                        }

                        writeData += "\n\n";

                        file.Write((void*)writeData.c_str(), writeData.size());
                    }
                }
            }
        }
        catch(const std::exception& e) {}
        
        LOGGER_LOG_INFO("Parsed batch %d/%d", i + 1, batchItemNames.size());
        SleepMS(10);
    }

    file.Close();
    LOGGER_LOG_INFO("Fetch wiki finished");
}

void GenerateItemTxtFromDat(uint32 serializeUntil = 0)
{
    File file;
    if(!file.Open(GetProgramPath() + "/items_generated.txt", FILE_MODE_WRITE)) {
        LOGGER_LOG_ERROR("Failed to open items_generated.txt");
        return;
    }

    ItemInfoManager* pItemMgr = GetItemInfoManager();
    if(!pItemMgr->LoadByItemsDat(GetProgramPath() + "/items.dat")) {
        LOGGER_LOG_ERROR("Failed to load items.dat");
        return;
    }

    if(serializeUntil == 0 || serializeUntil > pItemMgr->GetItemCount()) {
        serializeUntil = pItemMgr->GetItemCount();
    }

    for(uint32 inc = 0; inc < serializeUntil; ++inc) {
        ItemInfo* pItem = pItemMgr->GetItemByID(inc);
        if(!pItem) {
            LOGGER_LOG_ERROR("Item %d not found skipping...?", inc);
            continue;
        }

        string itemStr;
        if(pItem->name.find("null_item") != string::npos) {
            uint32 startID = pItem->id;
            uint32 endID = pItem->id + 1;

            while(inc + 1 < pItemMgr->GetItemCount()) {
                ItemInfo* pNextItem = pItemMgr->GetItemByID(inc + 1);
                if(!pNextItem || pNextItem->name.find("null_item") == string::npos) {
                    break;
                }

                ++inc;
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
            itemStr += ToString(pItem->seed1) + "|";
            itemStr += ToString(pItem->seed2) + "|";
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

        if(pItem->flags2 != 0) {
            string itemFlagStr;

            for(uint8 i = 0; i < 26; ++i) {
                if(pItem->flags2 & (1 << i)) {
                    if(!itemFlagStr.empty()) {
                        itemFlagStr += ",";
                    }

                    itemFlagStr += Flag2ToStr(1 << i);
                }
            }

            itemStr += "set_flags2|" + itemFlagStr + "|\n";
        }

        if(pItem->fxFlags != 0) {
            string itemFlagStr;

            if(pItem->fxFlags & ITEM_FX_FLAG_MULTI_ANIM) {
                itemFlagStr += FxFlagToStr(ITEM_FX_FLAG_MULTI_ANIM) + "|";
                itemFlagStr += pItem->multiAnim1;
                itemFlagStr += "MULTI_ANIM_END|";
            }

            if(pItem->fxFlags & ITEM_FX_FLAG_MULTI_ANIM2) {
                itemFlagStr += FxFlagToStr(ITEM_FX_FLAG_MULTI_ANIM2) + "|";
                itemFlagStr += pItem->multiAnim2;
                itemFlagStr += "MULTI_ANIM2_END|";
            }

            if(pItem->fxFlags & ITEM_FX_FLAG_DUAL_LAYER) {
                itemFlagStr += FxFlagToStr(ITEM_FX_FLAG_DUAL_LAYER) + "|";
                itemFlagStr += ToString(pItem->dualAnimLayer.x) + "," + ToString(pItem->dualAnimLayer.y) + "|";
            }

            if(pItem->fxFlags & ITEM_FX_FLAG_OVERLAY_OBJECT) {
                itemFlagStr += FxFlagToStr(ITEM_FX_FLAG_OVERLAY_OBJECT) + "|";
                itemFlagStr += pItem->overlayTextureFile + "|";
            }

            if(pItem->fxFlags & ITEM_FX_FLAG_RENDER_FX_VARIANT_VERSION) {
                itemFlagStr += FxFlagToStr(ITEM_FX_FLAG_RENDER_FX_VARIANT_VERSION) + "|";
                itemFlagStr += ToString(pItem->variantVersionItem) + "|";
            }

            for(uint8 i = 0; i < 32; ++i) {
                if(pItem->fxFlags & (1 << i)) {
                    if(
                        pItem->fxFlags & ITEM_FX_FLAG_MULTI_ANIM ||
                        pItem->fxFlags & ITEM_FX_FLAG_MULTI_ANIM2 ||
                        pItem->fxFlags & ITEM_FX_FLAG_DUAL_LAYER ||
                        pItem->fxFlags & ITEM_FX_FLAG_OVERLAY_OBJECT ||
                        pItem->fxFlags & ITEM_FX_FLAG_RENDER_FX_VARIANT_VERSION
                    ) {
                        continue;
                    }

                    itemFlagStr += FxFlagToStr(1 << i) + "|";
                }
            }

            itemStr += "set_fx_flags|" + itemFlagStr + "\n";
        }

        if(!pItem->extraString.empty() || pItem->animMS != 200) {
            itemStr += "set_extra|" + pItem->extraString + "|" + ToString(pItem->animMS) + "|\n";
        }

        if(!pItem->configName.empty()) {
            itemStr += "set_config_name|" + pItem->configName + "|\n";
        }

        if(pItem->maxCanHold != 200) {
            itemStr += "set_max_hold|" + ToString(pItem->maxCanHold) + "|\n";
        }

        if(pItem->rarity == 999) {
            itemStr += "set_rarity|" + ToString(pItem->rarity) + "|\n";
        }

        if(!pItem->customizedPunchParameters.empty()) {
            itemStr += "set_custom_punch|" + pItem->customizedPunchParameters + "|\n";
        }

        file.Write(itemStr.data(), itemStr.size());
    }

    file.Close();
    LOGGER_LOG_INFO("Serialized item data to txt");
}

#include <iostream>
int main(int argc, char const* argv[])
{
    LOGGER_LOG_INFO("Choose the operation\n1) Fetch wiki\n2) Generate items.txt");
    string operation;
    std::cin >> operation;

    int32 oprID = 0;
    if(ToInt(operation, oprID) != TO_INT_SUCCESS) {
        LOGGER_LOG_ERROR("What did u entered LOL");
        return 0;
    }

    switch(oprID) {
        case 1: {
            LOGGER_LOG_INFO("Start from ItemID?: ");
            string startIDInput;
            std::cin >> startIDInput;
        
            uint32 startID = 0;
            if(ToUInt(startIDInput, startID) != TO_INT_SUCCESS) {
                LOGGER_LOG_ERROR("What did u entered LOL");
                return 0;
            }

            LOGGER_LOG_INFO("Generate ıntil ItemID? (0 for all):");
            string untilIDInput;
            std::cin >> untilIDInput;
        
            uint32 untilID = 0;
            if(ToUInt(untilIDInput, untilID) != TO_INT_SUCCESS) {
                LOGGER_LOG_ERROR("What did u entered LOL");
                return 0;
            }

            FetchWikiAndWrite(startID, untilID);
            break;
        }

        case 2: {
            LOGGER_LOG_INFO("Generate until ItemID? (0 for all): ");
            string untilIDInput;
            std::cin >> untilIDInput;
        
            uint32 untilID = 0;
            if(ToUInt(untilIDInput, untilID) != TO_INT_SUCCESS) {
                LOGGER_LOG_ERROR("What did u entered LOL");
                return 0;
            }
            GenerateItemTxtFromDat(untilID);
        }
        
        default:
            LOGGER_LOG_ERROR("Unknown operation");
            return 0;
    }
}
