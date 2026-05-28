#include "PlayModManager.h"
#include "../IO/File.h"
#include "../Utils/StringUtils.h"
#include "CharacterData.h"
#include "../Item/ItemInfoManager.h"
#include "../IO/Log.h"

uint32 StrToCharacterStateFlag(const string& flag)
{
    static const std::unordered_map<string, uint32> charFlagStrMap =
    {
        {"NO_CLIP", CHAR_STATE_NO_CLIP},
        {"DOUBLE_JUMP", CHAR_STATE_DOUBLE_JUMP},
        {"RENDER_EYES_ONLY", CHAR_STATE_RENDER_EYES_ONLY},
        {"NO_ARMS", CHAR_STATE_NO_ARMS},
        {"NO_FACE", CHAR_STATE_NO_FACE},
        {"NO_HEAD_AND_TORSO", CHAR_STATE_NO_HEAD_AND_TORSO},
        {"DEVIL_HORNS", CHAR_STATE_DEVIL_HORNS},
        {"HALO", CHAR_STATE_HALO},
        {"HIGH_JUMP", CHAR_STATE_HIGH_JUMP},
        {"FIREPROOF", CHAR_STATE_FIREPROOF},
        {"SPIKEPROOF", CHAR_STATE_SPIKEPROOF},
        {"FROZEN", CHAR_STATE_FROZEN},
        {"WORLD_LOCKED", CHAR_STATE_WORLD_LOCKED},
        {"DUCT_TAPED", CHAR_STATE_DUCT_TAPED},
        {"STINKY", CHAR_STATE_STINKY},
        {"SPARKLY", CHAR_STATE_SPARKLY},
        {"ZOMBIFIED", CHAR_STATE_ZOMBIFIED},
        {"ON_FIRE", CHAR_STATE_ON_FIRE},
        {"SHADOW_HAUNTED", CHAR_STATE_SHADOW_HAUNTED},
        {"IRRADIATED", CHAR_STATE_IRRADIATED},
        {"SPOTLIGHT", CHAR_STATE_SPOTLIGHT},
        {"PINEAPPLE1", CHAR_STATE_PINEAPPLE1},
        {"PINEAPPLE2", CHAR_STATE_PINEAPPLE2},
        {"PINEAPPLE_AURA", CHAR_STATE_PINEAPPLE_AURA},
        {"SSUPPORTER", CHAR_STATE_SSUPPORTER},
        {"SUPER_PINEAPPLE_AURA", CHAR_STATE_SUPER_PINEAPPLE_AURA},
        {"BALLOON_WAR_SHIELD", CHAR_STATE_BALLOON_WAR_SHIELD},
        {"SOAKED", CHAR_STATE_SOAKED}
    };

    auto it = charFlagStrMap.find(flag);
    return (it != charFlagStrMap.end()) ? it->second : 0;
}

uint32 StrToCharacterState2Flag(const string& flag)
{
    static const std::unordered_map<string, uint32> charFlag2StrMap =
    {
        {"WINTERFEST_CROWN_RED", CHAR_STATE2_WINTERFEST_CROWN_RED},
        {"WINTERFEST_CROWN_GREEN", CHAR_STATE2_WINTERFEST_CROWN_GREEN},
        {"WINTERFEST_CROWN_SILVER", CHAR_STATE2_WINTERFEST_CROWN_SILVER},
        {"WINTERFEST_CROWN_GOLD", CHAR_STATE2_WINTERFEST_CROWN_GOLD},
        {"CHARGE_PUNCH", CHAR_STATE2_CHARGE_PUNCH},
        {"VALENTINE", CHAR_STATE2_VALENTINE},
        {"ELEMENT_FIRE", CHAR_STATE2_ELEMENT_FIRE},
        {"ELEMENT_WATER", CHAR_STATE2_ELEMENT_WATER},
        {"ELEMENT_EARTH", CHAR_STATE2_ELEMENT_EARTH},
        {"GIANT_POT_O_GOLD_LEVEL_2", CHAR_STATE2_GIANT_POT_O_GOLD_LEVEL_2},
        {"SHRINK_ME", CHAR_STATE2_SHRINK_ME},
        {"MIND_CONTROL", CHAR_STATE2_MIND_CONTROL},
        {"PINEAPPLE_CHARM_SHIELD", CHAR_STATE2_PINEAPPLE_CHARM_SHIELD},
        {"GLOVE_OF_GIANTS", CHAR_STATE2_GLOVE_OF_GIANTS},
        {"FORCE_SHIELD", CHAR_STATE2_FORCE_SHIELD},
        {"TUTORIAL_ACTIVE", CHAR_STATE2_TUTORIAL_ACTIVE},
        {"SHOW_COMPOSER_GRID", CHAR_STATE2_SHOW_COMPOSER_GRID},
        {"MAD_HATTER", CHAR_STATE2_MAD_HATTER},
        {"SLOW_FALL", CHAR_STATE2_SLOW_FALL},
        {"BUMBLE_BOT_STAND_UP", CHAR_STATE2_BUMBLE_BOT_STAND_UP},
        {"NOT_DUPLICATE_PET", CHAR_STATE2_NOT_DUPLICATE_PET},
        {"HEAL_PARTICLE", CHAR_STATE2_HEAL_PARTICLE},
        {"ENCHANTED_ROBE_SHAPE1", CHAR_STATE2_ENCHANTED_ROBE_SHAPE1},
        {"ENCHANTED_ROBE_SHAPE2", CHAR_STATE2_ENCHANTED_ROBE_SHAPE2},
        {"ENCHANTED_ROBE_SHAPE3", CHAR_STATE2_ENCHANTED_ROBE_SHAPE3},
        {"CHARACTER_IS_PAINTED", CHAR_STATE2_CHARACTER_IS_PAINTED},
        {"BALLOON_BUNNY", CHAR_STATE2_BALLOON_BUNNY},
        {"FISH_SQUISHED_WEBBED_HAND", CHAR_STATE2_FISH_SQUISHED_WEBBED_HAND},
        {"SPACE_FACED_CHEST", CHAR_STATE2_SPACE_FACED_CHEST},
        {"ILL_FILLED_SCALE", CHAR_STATE2_ILL_FILLED_SCALE},
        {"SICK_LICKED_FACEITEM", CHAR_STATE2_SICK_LICKED_FACEITEM},
        {"DIRT_GROSS_BEAN", CHAR_STATE2_DIRT_GROSS_BEAN}
    };

    auto it = charFlag2StrMap.find(flag);
    return (it != charFlag2StrMap.end()) ? it->second : 0;
}

PlayModManager::PlayModManager()
{
}

PlayModManager::~PlayModManager()
{
}

bool PlayModManager::Load(const string &filePath)
{
    File file;
    if(!file.Open(filePath)) {
        return false;
    }

    uint32 fileSize = file.GetSize();
    string fileData(fileSize, '\0');
    if(file.Read(fileData.data(), fileSize) != fileSize) {
        return false;
    }

    auto lines = Split(fileData, '\n');

    for(auto& line : lines) {
        if(line.empty() || line[0] == '#') {
            continue;
        }

        auto args = Split(line, '|');
        if(args[0] == "add_playmod") {
            PlayMod playMod;
            playMod.m_displayItem = (uint16)ToUInt(args[1]);
            playMod.m_name = args[2];
            playMod.m_addMessage = args[3];
            playMod.m_removeMessage = args[4];

            playMod.m_modType = ePlayModType(m_playMods.size() + 1);
            
            m_playMods.push_back(std::move(playMod));
        }

        if(args[0] == "set_char_flags") {
            for(uint8 i = 1; i < args.size(); ++i)
            {
                m_playMods.back().m_charState |= StrToCharacterStateFlag(args[i]);
            }
        }

        if(args[0] == "set_char2_flags") {
            for(uint8 i = 1; i < args.size(); ++i)
            {
                m_playMods.back().m_char2State |= StrToCharacterState2Flag(args[i]);
            }
        }

        if(args[0] == "set_skin_color") {
            m_playMods.back().m_skinColor = ToColor(args[1], ',');
        }

        if(args[0] == "set_timer") {
            m_playMods.back().m_durationTime = ToInt(args[1]);
        }

        if(args[0] == "set_punch_damage") {
            m_playMods.back().m_punchDamage = ToFloat(args[1]);
        }

        if(args[0] == "set_punch_power") {
            m_playMods.back().m_punchPower = ToFloat(args[1]);
        }

        if(args[0] == "set_build_range") {
            m_playMods.back().m_buildRange = ToInt(args[1]);
        }

        if(args[0] == "set_speed") {
            m_playMods.back().m_speed = ToFloat(args[1]);
        }

        if(args[0] == "set_punch_type") {
            m_playMods.back().m_punchType = (uint8)ToInt(args[1]);
        }

        if(args[0] == "set_items") {
            ItemInfoManager* pItemMgr = GetItemInfoManager();
            ePlayModType modType = m_playMods.back().m_modType;

            for(uint16 i = 1; i < args.size(); ++i) {
                uint32 itemID = 0;
                if(ToUInt(args[i], itemID) != TO_INT_SUCCESS) {
                    continue;
                }

                ItemInfo* pItem = pItemMgr->GetItemByID(itemID);
                if(!pItem) {
                    LOGGER_LOG_ERROR("Failed to setup playmod for item %d its not exists", itemID);
                    continue;
                }

                pItem->playModType = modType;
            }
        }
    }

    return true;
}

PlayMod* PlayModManager::GetPlayMod(ePlayModType type)
{
    if(type > m_playMods.size()) {
        return nullptr;
    }

    for(auto& playmod : m_playMods) {
        if(playmod.m_modType == type) {
            return &playmod;
        }
    }

    return nullptr;
}

PlayModManager* GetPlayModManager() { return PlayModManager::GetInstance(); }