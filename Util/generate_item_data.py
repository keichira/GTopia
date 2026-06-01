import sys
from pathlib import Path
from item_info_manager import ItemInfoManager

ITEM_FLAG_STR = [
    "FLIPPABLE",
    "EDITABLE",
    "SEEDLESS",
    "PERMANENT",
    "DROPLESS",
    "NOSELF",
    "RANDGROW",
    "WORLDLOCKED",
    "BETA",
    "AUTOPICKUP",
    "MOD",
    "PUBLIC",
    "FOREGROUND",
    "HOLIDAY",
    "UNTRADEABLE",
]

def ItemFlagToStr(flag: int) -> str:
    if flag < 0 or flag >= len(ITEM_FLAG_STR):
        return "UNTRADEABLE"
    return ITEM_FLAG_STR[flag]

ITEM_FX_STR = [
    "MULTI_ANIM",
    "PING_PONG_ANIM",
    "OVERLAY_OBJECT",
    "OFFSET_UP",
    "DUAL_LAYER",
    "MULTI_ANIM2",
    "USE_SKIN_TINT",
    "SEED_TINT_LAYER1",
    "SEED_TINT_LAYER2",
    "RAINBOW_TINT_LAYER1",
    "RAINBOW_TINT_LAYER2",
    "GLOW",
    "NO_ARMS",
    "FRONT_ARM_PUNCH",
    "RENDER_OFFHAND",
    "SLOWFALL_OBJECT",
    "REPLACEMENT_SPRITE",
    "ORB_FLOAT",
    "RENDER_FX_VARIANT_VERSION",
]

def FxFlagToStr(flag: int) -> str:
    if flag < 0 or flag >= len(ITEM_FX_STR):
        return ""
    return ITEM_FX_STR[flag]

ITEM_FLAG2_STR = [
    "ROBOT_DEADLY",
    "ROBOT_SHOOT_LEFT",
    "ROBOT_SHOOT_RIGHT",
    "ROBOT_SHOOT_DOWN",
    "ROBOT_SHOOT_UP",
    "ROBOT_CAN_SHOOT",
    "ROBOT_LAVA",
    "ROBOT_POINTY",
    "ROBOT_SHOOT_DEADLY",
    "GUILD_ITEM",
    "GUILD_FLAG",
    "STARSHIP_HELM",
    "STARSHIP_REACTOR",
    "STARSHIP_VIEWSCREEN",
    "SMOD",
    "TILE_DEADLY_IF_ON",
    "LONG_HAND_ITEM64x32",
    "GEMLESS",
    "TRANSMUTABLE",
    "DUNGEON_ITEM",
    "ONE_IN_WORLD",
    "ONLY_FOR_WORLD_OWNER",
    "PVE_MELEE",
    "PVE_RANGED",
    "PVE_AUTOAIM",
    "NO_UPGRADE",
]

def Flag2ToStr(flag: int) -> str:
    if flag < 0 or flag >= len(ITEM_FLAG2_STR):
        return ""
    return ITEM_FLAG2_STR[flag]

VISUAL_EFFECT_STR = [
    "VISUAL_EFFECT_NORMAL",
    "VISUAL_EFFECT_FLAME_LICK",
    "VISUAL_EFFECT_SMOKING",
    "VISUAL_EFFECT_GLOW_TINT",
    "VISUAL_EFFECT_ANIM",
    "VISUAL_EFFECT_BUBBLES",
    "VISUAL_EFFECT_PET",
    "VISUAL_EFFECT_PET_ANIM",
    "VISUAL_EFFECT_NO_ARMS",
    "VISUAL_EFFECT_WAVEY",
    "VISUAL_EFFECT_WAVEY_ANIM",
    "VISUAL_EFFECT_BOTHARMS",
    "VISUAL_EFFECT_LOWHAIR",
    "VISUAL_EFFECT_UNDERFACE",
    "VISUAL_EFFECT_SKINTINT",
    "VISUAL_EFFECT_MASK",
    "VISUAL_EFFECT_ANIM_MASK",
    "VISUAL_EFFECT_LOWHAIR_MASK",
    "VISUAL_EFFECT_GHOST",
    "VISUAL_EFFECT_PULSE",
    "VISUAL_EFFECT_COLORIZE",
    "VISUAL_EFFECT_COLORIZE_TO_SHIRT",
    "VISUAL_EFFECT_COLORIZE_ANIM",
    "VISUAL_EFFECT_HIGHFACE",
    "VISUAL_EFFECT_HIGHFACE_ANIM",
    "VISUAL_EFFECT_RAINBOW_SHIFT",
    "VISUAL_EFFECT_BACKFORE",
    "VISUAL_EFFECT_COLORIZE_WITH_SKIN",
    "VISUAL_EFFECT_NO_RENDER",
    "VISUAL_EFFECT_SPIN",
    "VISUAL_EFFECT_OFFHAND",
    "VISUAL_EFFECT_WINGED",
    "VISUAL_EFFECT_SINK",
    "VISUAL_EFFECT_DARKNESS",
    "VISUAL_EFFECT_LIGHTSOURCE",
    "VISUAL_EFFECT_LIGHT_IF_ON",
    "VISUAL_EFFECT_DISCOLOR",
    "VISUAL_EFFECT_STEP_SPIN",
    "VISUAL_EFFECT_PETCOLORED",
    "VISUAL_EFFECT_SILKFOOT",
    "VISUAL_EFFECT_TILTY",
    "VISUAL_EFFECT_TILTY_DARK",
    "VISUAL_EFFECT_NEXT_FRAME_IF_ON",
    "VISUAL_EFFECT_WOBBLE",
    "VISUAL_EFFECT_SCROLL",
    "VISUAL_EFFECT_LIGHTSOURCE_PULSE",
    "VISUAL_EFFECT_BUBBLE_MACHINE",
    "VISUAL_EFFECT_VERYLOWHAIR",
    "VISUAL_EFFECT_VERYLOWHAIR_MASK",
]

def ItemVisualEffectToStr(v: int) -> str:
    return VISUAL_EFFECT_STR[v] if 0 <= v < len(VISUAL_EFFECT_STR) else "VISUAL_EFFECT_NORMAL"

ITEM_STORAGE_STR = [
    "STORAGE_SINGLE_FRAME_ALONE",
    "STORAGE_SINGLE_FRAME",
    "STORAGE_SMART_EDGE",
    "STORAGE_SMART_EDGE_HORIZ",
    "STORAGE_SMART_CLING",
    "STORAGE_SMART_EDGE_OUTER",
    "STORAGE_RANDOM",
    "STORAGE_SMART_EDGE_VERT",
    "STORAGE_SMART_EDGE_HORIZ_CAVE",
    "STORAGE_SMART_CLING2",
    "STORAGE_SMART_EDGE_DIAGON",
]

def ItemStorageTypeToStr(type_):
    if 0 <= type_ < len(ITEM_STORAGE_STR):
        return ITEM_STORAGE_STR[type_]
    return "STORAGE_SINGLE_FRAME_ALONE"

ITEM_COLLISION_STR = [
    "COLLISION_NONE",
    "COLLISION_SOLID",
    "COLLISION_JUMP_THROUGH",
    "COLLISION_GATEWAY",
    "COLLISION_IF_OFF",
    "COLLISION_ONE_WAY",
    "COLLISION_VIP",
    "COLLISION_JUMP_DOWN",
    "COLLISION_ADVENTURE",
    "COLLISION_IF_ON",
    "COLLISION_FACTION",
    "COLLISION_GUILD",
    "COLLISION_CLOUD",
    "COLLISION_FRIENDS_ENTRANCE",
]

def ItemCollisionTypeToStr(type_):
    if 0 <= type_ < len(ITEM_COLLISION_STR):
        return ITEM_COLLISION_STR[type_]
    return "COLLISION_NONE"

ITEM_MATERIAL_STR = [
    "MATERIAL_WOOD",
    "MATERIAL_GLASS",
    "MATERIAL_ROCK",
    "MATERIAL_METAL",
]

def ItemMaterialToStr(type_):
    if 0 <= type_ < len(ITEM_MATERIAL_STR):
        return ITEM_MATERIAL_STR[type_]
    return "MATERIAL_WOOD"

ITEM_ELEMENT_STR = [
    "EARTH",
    "FIRE",
    "AIR",
    "WATER",
    "NONE"
]

def ItemElementToStr(type_):
    if 0 <= type_ < len(ITEM_ELEMENT_STR):
        return ITEM_ELEMENT_STR[type_]
    return "NONE"

ITEM_TYPE_STR = [
    "TYPE_FIST",
    "TYPE_WRENCH",
    "TYPE_USER_DOOR",
    "TYPE_LOCK",
    "TYPE_GEMS",
    "TYPE_TREASURE",
    "TYPE_DEADLY",
    "TYPE_TRAMPOLINE",
    "TYPE_CONSUMABLE",
    "TYPE_GATEWAY",
    "TYPE_SIGN",
    "TYPE_SFX_WITH_EXTRA_FRAME",
    "TYPE_BOOMBOX",
    "TYPE_DOOR",
    "TYPE_PLATFORM",
    "TYPE_BEDROCK",
    "TYPE_LAVA",
    "TYPE_NORMAL",
    "TYPE_BACKGROUND",
    "TYPE_SEED",
    "TYPE_CLOTHES",
    "TYPE_NORMAL_WITH_EXTRA_FRAME",
    "TYPE_BACKGD_SFX_EXTRA_FRAME",
    "TYPE_BACK_BOOMBOX",
    "TYPE_BOUNCY",
    "TYPE_POINTY",
    "TYPE_PORTAL",
    "TYPE_CHECKPOINT",
    "TYPE_MUSICNOTE",
    "TYPE_ICE",
    "TYPE_RACE_FLAG",
    "TYPE_SWITCHEROO",
    "TYPE_CHEST",
    "TYPE_MAILBOX",
    "TYPE_BULLETIN",
    "TYPE_PINATA",
    "TYPE_COMPONENT",
    "TYPE_DICE",
    "TYPE_PROVIDER",
    "TYPE_LAB",
    "TYPE_ACHIEVEMENT",
    "TYPE_WEATHER_MACHINE",
    "TYPE_SCOREBOARD",
    "TYPE_SUNGATE",
    "TYPE_PROFILE",
    "TYPE_DEADLY_IF_ON",
    "TYPE_HEART_MONITOR",
    "TYPE_DONATION_BOX",
    "TYPE_TOYBOX",
    "TYPE_MANNEQUIN",
    "TYPE_CAMERA",
    "TYPE_MAGICEGG",
    "TYPE_TEAM",
    "TYPE_GAME_GEN",
    "TYPE_XENONITE",
    "TYPE_DRESSUP",
    "TYPE_CRYSTAL",
    "TYPE_BURGLAR",
    "TYPE_COMPACTOR",
    "TYPE_SPOTLIGHT",
    "TYPE_WIND",
    "TYPE_DISPLAY_BLOCK",
    "TYPE_VENDING",
    "TYPE_FISHTANK",
    "TYPE_PETFISH",
    "TYPE_SOLAR",
    "TYPE_FORGE",
    "TYPE_GIVING_TREE",
    "TYPE_GIVING_TREE_STUMP",
    "TYPE_STEAMPUNK",
    "TYPE_STEAM_LAVA_IF_ON",
    "TYPE_STEAM_ORGAN",
    "TYPE_TAMAGOTCHI",
    "TYPE_SEWING",
    "TYPE_FLAG",
    "TYPE_LOBSTER_TRAP",
    "TYPE_ARTCANVAS",
    "TYPE_BATTLE_CAGE",
    "TYPE_PET_TRAINER",
    "TYPE_STEAM_ENGINE",
    "TYPE_LOCK_BOT",
    "TYPE_WEATHER_SPECIAL",
    "TYPE_SPIRIT_STORAGE",
    "TYPE_DISPLAY_SHELF",
    "TYPE_VIP_DOOR",
    "TYPE_CHAL_TIMER",
    "TYPE_CHAL_FLAG",
    "TYPE_FISH_MOUNT",
    "TYPE_PORTRAIT",
    "TYPE_WEATHER_SPECIAL2",
    "TYPE_FOSSIL",
    "TYPE_FOSSIL_PREP",
    "TYPE_DNA_MACHINE",
    "TYPE_BLASTER",
    "TYPE_VALHOWLA",
    "TYPE_CHEMSYNTH",
    "TYPE_CHEMTANK",
    "TYPE_STORAGE",
    "TYPE_OVEN",
    "TYPE_SUPER_MUSIC",
    "TYPE_GEIGERCHARGE",
    "TYPE_ADVENTURE_RESET",
    "TYPE_TOMB_ROBBER",
    "TYPE_FACTION",
    "TYPE_RED_FACTION",
    "TYPE_GREEN_FACTION",
    "TYPE_BLUE_FACTION",
    "TYPE_ARTIFACT",
    "TYPE_TRAMPOLINE_MOMENTUM",
    "TYPE_FISHGOTCHI_TANK",
    "TYPE_FISHING_BLOCK",
    "TYPE_SUCKER",
    "TYPE_PLANTER",
    "TYPE_ROBOT",
    "TYPE_COMMAND",
    "TYPE_LUCKY_TICKET",
    "TYPE_STATS_BLOCK",
    "TYPE_FIELD_NODE",
    "TYPE_OUIJA_BOARD",
    "TYPE_ARCHITECT_MACHINE",
    "TYPE_STARSHIP",
    "TYPE_AUTODELETE",
    "TYPE_BOOMBOX2",
    "TYPE_AUTO_ACTION_BREAK",
    "TYPE_AUTO_ACTION_HARVEST",
    "TYPE_AUTO_ACTION_HARVEST_SUCK",
    "TYPE_LIGHTNING_CLOUD",
    "TYPE_PHASED_BLOCK",
    "TYPE_MUD",
    "TYPE_ROOT_CUTTING",
    "TYPE_PASSWORD_STORAGE",
    "TYPE_PHASED_BLOCK_2",
    "TYPE_BOMB",
    "TYPE_PVE_NPC",
    "TYPE_INFINITY_WEATHER_MACHINE",
    "TYPE_SLIME",
    "TYPE_ACID",
    "TYPE_COMPLETIONIST",
    "TYPE_PUNCH_TOGGLE",
    "TYPE_ANZU_BLOCK",
    "TYPE_FEEDING_BLOCK",
    "TYPE_KRANKENS_BLOCK",
    "TYPE_FRIENDS_ENTRANCE",
    "TYPE_PEARLS",
]

ITEM_BODY_PART_STR = [
    "HAT",
    "SHIRT",
    "PANT",
    "SHOE",
    "FACEITEM",
    "HAND",
    "BACK",
    "HAIR",
    "CHESTITEM",
]

def ItemBodyPartToStr(type_):
    if 0 <= type_ < len(ITEM_BODY_PART_STR):
        return ITEM_BODY_PART_STR[type_]
    return "HAIR"

def ItemTypeToStr(type_):
    if 0 <= type_ < len(ITEM_TYPE_STR):
        return ITEM_TYPE_STR[type_]
    return "TYPE_NORMAL"

def generate_item_txt_from_dat(serialize_until: int = 0, file_path: str = "items.dat"):
    mgr = ItemInfoManager()
    mgr.load_from_file(file_path)

    item_count = len(mgr.items)

    if serialize_until == 0 or serialize_until > item_count:
        serialize_until = item_count

    with open("items.txt", "w", encoding="utf-8") as file:

        inc = 0
        while inc < serialize_until:

            p_item = mgr.get_item_by_id(inc)

            if not p_item:
                print(f"Item {inc} not found skipping...")
                inc += 1
                continue

            item_str = ""

            if "null_item" in p_item.name:
                start_id = p_item.id
                end_id = p_item.id + 1

                while inc + 1 < item_count:
                    next_item = mgr.get_item_by_id(inc + 1)
                    if not next_item or "null_item" not in next_item.name:
                        break

                    inc += 1
                    end_id = next_item.id

                file.write(f"make_null|{start_id}|{end_id}|\n\n")
                inc += 1
                continue

            elif p_item.type == 20:
                item_str += "add_cloth|"
                item_str += f"{p_item.id}|{p_item.name}|"
                item_str += f"{ItemMaterialToStr(p_item.material)}|"
                item_str += f"{p_item.texture_file}|"
                item_str += f"{p_item.texture_x}|{p_item.texture_y}|"
                item_str += f"{ItemVisualEffectToStr(p_item.visual_effect)}|"
                item_str += f"{ItemStorageTypeToStr(p_item.storage)}|"
                item_str += f"{ItemBodyPartToStr(p_item.body_part)}|\n"

            elif p_item.type == 19:
                item_str += (
                    f"set_seed|{p_item.seed1}|{p_item.seed2}|"
                    f"{p_item.seed_bg_color[0]},{p_item.seed_bg_color[1]},{p_item.seed_bg_color[2]},{p_item.seed_bg_color[3]}|"
                    f"{p_item.seed_fg_color[0]},{p_item.seed_fg_color[1]},{p_item.seed_fg_color[2]},{p_item.seed_fg_color[3]}|\n\n"
                )

                file.write(item_str)
                inc += 1
                continue

            else:
                item_str += "add_item|"
                item_str += f"{p_item.id}|{p_item.name}|"
                item_str += f"{ItemTypeToStr(p_item.type)}|"
                item_str += f"{ItemMaterialToStr(p_item.material)}|"
                item_str += f"{p_item.texture_file}|"
                item_str += f"{p_item.texture_x}|{p_item.texture_y}|"
                item_str += f"{ItemVisualEffectToStr(p_item.visual_effect)}|"
                item_str += f"{ItemStorageTypeToStr(p_item.storage)}|"
                item_str += f"{ItemCollisionTypeToStr(p_item.collision_type)}|"
                item_str += f"{p_item.hp // 6}|{p_item.restore_time}|1|{p_item.texture_hash}|\n"

            if p_item.flags != 0:
                flag_list = []
                for i in range(16):
                    if p_item.flags & (1 << i):
                        flag_list.append(ItemFlagToStr(i))

                item_str += "set_flags|" + "|".join(flag_list) + "|\n"

            if p_item.flags2 != 0:
                flag_list = []
                for i in range(26):
                    if p_item.flags2 & (1 << i):
                        flag_list.append(Flag2ToStr(i))

                item_str += "set_flags2|" + "|".join(flag_list) + "|\n"

            if p_item.fx_flags != 0:
                fx = []

                if p_item.fx_flags & (1 << 0):
                    fx.append(FxFlagToStr(0))
                    fx.append(p_item.multi_anim1)
                    fx.append("MULTI_ANIM_END")

                if p_item.fx_flags & (1 << 5):
                    fx.append(FxFlagToStr(5))
                    fx.append(p_item.multi_anim2)
                    fx.append("MULTI_ANIM2_END")

                if p_item.fx_flags & (1 << 4):
                    fx.append(FxFlagToStr(4))
                    fx.append(f"{p_item.dual_anim_layer[0]},{p_item.dual_anim_layer[1]}")

                if p_item.fx_flags & (1 << 2):
                    fx.append(FxFlagToStr(2))
                    fx.append(p_item.overlay_texture_file)

                if p_item.fx_flags & (1 << 20):
                    fx.append(FxFlagToStr(20))
                    fx.append(str(p_item.variant_version_item))

                SPECIAL_MASK = (
                    1 << 0 |
                    1 << 5 |
                    1 << 4 |
                    1 << 2 |
                    1 << 20
                )

                for i in range(32):            
                    if p_item.fx_flags & (1 << i):
                        if p_item.fx_flags & SPECIAL_MASK:
                            continue
            
                        name = FxFlagToStr(i)
                        if name:
                            fx.append(name)

                item_str += "set_fx_flags|" + "|".join(fx) + "|\n"

            if p_item.extra_string or p_item.anim_ms != 200 or p_item.extra_string_hash > 0:
                item_str += f"set_extra|{p_item.extra_string}|{p_item.anim_ms}|{p_item.extra_string_hash}|\n"

            if p_item.config_name:
                item_str += f"set_config_name|{p_item.config_name}|\n"

            if p_item.max_can_hold != 200:
                item_str += f"set_max_hold|{p_item.max_can_hold}|\n"

            if p_item.rarity != 0:
                item_str += f"set_rarity|{p_item.rarity}|\n"

            if p_item.customized_punch_parameters:
                item_str += f"set_custom_punch|{p_item.customized_punch_parameters}|\n"

            file.write(item_str)
            inc += 1

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python generate_item_data.py <items_dat_path>")
        sys.exit(1)

    path = Path(sys.argv[1]).expanduser().resolve()

    generate_item_txt_from_dat(0, path)