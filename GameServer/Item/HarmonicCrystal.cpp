#include "HarmonicCrystal.h"
#include "Item/ItemUtils.h"
#include "../World/World.h"

/**
 * 1 -> red crystal
 * 2 -> green crystal
 * 3 -> blue crystal
 * 4 -> white crystal
 * 5 -> black crystal
 */

HarmonicCrystal gHarmonicCrystal;

HarmonicCrystal::HarmonicCrystal()
{
    AddRecipe({ "11224", ITEM_ID_YELLOW_DIAMOND, 1 });
    AddRecipe({ "22444", ITEM_ID_BLUE_CRYSTAL_WINGS, 1 });
    AddRecipe({ "12344", ITEM_ID_CELESTIAL_KALEIDOSCOPE, 1 });
    AddRecipe({ "11133", ITEM_ID_PURPLE_CRYSTAL_SKIRT, 1 });
    AddRecipe({ "45555", ITEM_ID_LUMINOUS_EYES, 1 });
    AddRecipe({ "12234", ITEM_ID_KALEIDOSCOPIC_WALLPAPER, 10 });
    AddRecipe({ "22333", ITEM_ID_CRYSTAL_TREE, 3 });
    AddRecipe({ "11233", ITEM_ID_CRYSTAL_SKULL, 1 });
    AddRecipe({ "33444", ITEM_ID_GREEN_CRYSTAL_WINGS, 1 });
    AddRecipe({ "11124", ITEM_ID_STATIC_HAIR, 1 });
    AddRecipe({ "12233", ITEM_ID_STAINED_GLASS, 10 });
    AddRecipe({ "55555", ITEM_ID_BLACK_CRYSTAL_DRAGON, 1 });
    AddRecipe({ "11334", ITEM_ID_SPIKEY_ANIME_HAIR, 1 });
    AddRecipe({ "11222", ITEM_ID_GOLDEN_CRYSTAL_SKIRT, 1 });
    AddRecipe({ "14445", ITEM_ID_THE_GUNGNIR, 1 });
    AddRecipe({ "12355", ITEM_ID_CRYSTAL_SMITHING_TOOLS, 1 });
    AddRecipe({ "22244", ITEM_ID_EMERALD_CHOKER, 1 });
    AddRecipe({ "11122", ITEM_ID_GILDED_FRAME, 20 });
    AddRecipe({ "23444", ITEM_ID_AQUA_CRYSTAL_WINGS, 1 });
    AddRecipe({ "11333", ITEM_ID_CRYSTAL_SPIKES, 5 });
    AddRecipe({ "12444", ITEM_ID_GOLDEN_CRYSTAL_WINGS, 1 });
    AddRecipe({ "11234", ITEM_ID_SHIFTY_BLOCK, 5 });
    AddRecipe({ "33344", ITEM_ID_CRYSTAL_SHARD_NECKLACE, 1 });
    AddRecipe({ "11444", ITEM_ID_RED_CRYSTAL_WINGS, 1 });
    AddRecipe({ "11125", ITEM_ID_DEMON_CRYSTAL, 1 });
    AddRecipe({ "12223", ITEM_ID_CELESTIAL_FRAME, 10 });
    AddRecipe({ "13344", ITEM_ID_HARMONIC_CHIMES, 1 });
    AddRecipe({ "22455", ITEM_ID_CRYSTAL_GLAIVE, 1 });
    AddRecipe({ "33333", ITEM_ID_SAPPHIRE_BLOCK, 1 });
    AddRecipe({ "11555", ITEM_ID_DEMONIC_ARM, 1 });
    AddRecipe({ "11144", ITEM_ID_AMETHYST_CHOKER, 1 });
    AddRecipe({ "22344", ITEM_ID_CRYSTAL_CLOCK, 1 });
    AddRecipe({ "11225", ITEM_ID_GOLDEN_CAVE_CRYSTAL, 1 });
    AddRecipe({ "22233", ITEM_ID_AQUA_CRYSTAL_SKIRT, 1 });
    AddRecipe({ "44555", ITEM_ID_DNA_EXTRACTOR, 1 });
    AddRecipe({ "12333", ITEM_ID_OPAL_BLOCK, 1 });
    AddRecipe({ "13444", ITEM_ID_PURPLE_CRYSTAL_WINGS, 1 });
    AddRecipe({ "11244", ITEM_ID_PLASMA_GLOBE, 1 });
    AddRecipe({ "12234", ITEM_ID_KALEIDOSCOPIC_WALLPAPER, 10 });
    AddRecipe({ "11134", ITEM_ID_CRYSTAL_GATE, 1 });
    AddRecipe({ "11335", ITEM_ID_AMETHYST_BLOCK, 1 });
    AddRecipe({ "22244", ITEM_ID_EMERALD_CHOKER, 1 });
    AddRecipe({ "11223", ITEM_ID_STEAM_LAMP, 3 });
    

    //AddRecipe({ "11145", ITEM_ID_HEARTSTONE, 1 });
}

HarmonicCrystalRecipe* HarmonicCrystal::GetRecipe(const string& crystals)
{
    if(crystals.size() != 5)
        return nullptr;

    auto it = m_recipes.find(EncodeRecipe(crystals));
    if(it != m_recipes.end())
        return &it->second;

    return nullptr;
}

char HarmonicCrystal::GetCrystalCodeFromID(int32 itemID)
{
    return '1' + ((itemID - ITEM_ID_RED_CRYSTAL) / 2);
}

int16 HarmonicCrystal::GetChiAccuracy(TileInfo* pTile, World* pWorld)
{
    if(pTile && pTile->GetBG() == ITEM_ID_CHI_HARMONIZER)
        return 30;

    if(pWorld)
    {
        TileInfo* pLockTile = pWorld->GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);
        if(pLockTile && pLockTile->GetFG() == ITEM_ID_HARMONIC_LOCK)
            return 15;
    }

    return 6;
}

uint16 HarmonicCrystal::EncodeRecipe(const string& crystals)
{
    uint16 key = 0;

    for(char c : crystals)
    {
        key = key * 5 + (c - '1');
    }

    return key;
}

void HarmonicCrystal::AddRecipe(const HarmonicCrystalRecipe& recipe)
{
    m_recipes[EncodeRecipe(recipe.crystals)] = recipe;
}
