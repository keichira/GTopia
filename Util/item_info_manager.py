def xor_cipher_string(s: str, secret: str, item_id: int) -> str:
    secret_len = len(secret)
    if secret_len == 0:
        return s

    idx = item_id % secret_len
    data = bytearray(s.encode("utf-8"))

    for i in range(len(data)):
        data[i] ^= ord(secret[idx])
        idx += 1
        if idx >= secret_len:
            idx = 0

    return data.decode("utf-8", errors="ignore")

class Buffer:
    def __init__(self, data):
        self.buf = memoryview(data)
        self.i = 0

    def u8(self):
        v = self.buf[self.i]
        self.i += 1
        return v

    def u16(self):
        v = int.from_bytes(self.buf[self.i:self.i+2], "little", signed=False)
        self.i += 2
        return v

    def u32(self):
        v = int.from_bytes(self.buf[self.i:self.i+4], "little", signed=False)
        self.i += 4
        return v

    def i8(self):
        v = int.from_bytes(self.buf[self.i:self.i+1], "little", signed=True)
        self.i += 1
        return v

    def i16(self):
        v = int.from_bytes(self.buf[self.i:self.i+2], "little", signed=True)
        self.i += 2
        return v

    def i32(self):
        v = int.from_bytes(self.buf[self.i:self.i+4], "little", signed=True)
        self.i += 4
        return v
    
    def str(self):
        length = self.u16()
        s = self.buf[self.i:self.i+length].tobytes().decode("utf-8", "ignore")
        self.i += length
        return s
    
class ItemInfo:
    def __init__(self):
        self.id = 0
        self.flags = 0
        self.type = 0
        self.material = 0

        self.name = ""
        self.texture_file = ""
        self.texture_hash = 0

        self.visual_effect = 0
        self.cooking_time = -1

        self.texture_x = 0
        self.texture_y = 0
        self.storage = 0

        self.layer = 0
        self.collision_type = 0
        self.hp = 0
        self.restore_time = 0
        self.body_part = 0
        self.rarity = 0
        self.max_can_hold = 200

        self.extra_string = ""
        self.extra_string_hash = 0

        self.anim_ms = 200

        self.pet_name = ""
        self.pet_sub_name = ""
        self.pet_end_name = ""
        self.pet_power_name = ""

        self.seed_bg = 0
        self.seed_fg = 0
        self.tree_bg = 0
        self.tree_fg = 0

        self.seed_bg_color = 0
        self.seed_fg_color = 0

        self.seed1 = 0
        self.seed2 = 0
        self.grow_time = 31

        self.fx_flags = 0
        self.multi_anim1 = ""
        self.overlay_texture_file = ""
        self.multi_anim2 = ""

        self.dual_anim_layer = (0, 0)

        self.flags2 = 0
        self.client_data = [0] * 15

        self.tile_range = 0
        self.pile_size = 0
        self.customized_punch_parameters = ""

        self.extra_slot_counter = 0
        self.extra_slot_body_parts = [0] * 9

        self.light_source_range = 0
        self.variant_version_item = 0

        self.chair_info = type("Chair", (), {})()
        self.chair_info.enabled = 0
        self.chair_info.player_offset = (0, 0)
        self.chair_info.arm_pos = (0, 0)
        self.chair_info.arm_offset = (0, 0)
        self.chair_info.arm_texture = ""
        
        self.random_sprite_info = type("RS", (), {})()
        self.random_sprite_info.enabled = 0
        self.random_sprite_info.offset_mod = 0
        self.random_sprite_info.chance = 0.0

        self.config_name = ""
        self.other_player_hit_particle = 0
        self.config_name_hash = 0

        self.hidden_parts_flags = 0
        self.can_transform = 0
        self.description = ""

        self.slippery_type = 0

    @staticmethod
    def deserialize(buf, version):
        item = ItemInfo()
    
        item.id = buf.u32()
        item.flags = buf.u16()
        item.type = buf.u8()
        item.material = buf.u8()
    
        if version < 3:
            item.name = buf.str()
        else:
            raw = buf.str()
            item.name = xor_cipher_string(raw, "PBG892FXX982ABC*", item.id)
    
        item.texture_file = buf.str()
        item.texture_hash = buf.u32()
        item.visual_effect = buf.u8()
        item.cooking_time = buf.i32()
    
        item.texture_x = buf.u8()
        item.texture_y = buf.u8()
        item.storage = buf.u8()
    
        item.layer = buf.i8()
        item.collision_type = buf.u8()
        item.hp = buf.u8()
        item.restore_time = buf.i32()
        item.body_part = buf.u8()
        item.rarity = buf.i16()
        item.max_can_hold = buf.u8()
    
        item.extra_string = buf.str()
        item.extra_string_hash = buf.u32()
        item.anim_ms = buf.i32()
    
        if version > 3:
            item.pet_name = buf.str()
            item.pet_sub_name = buf.str()
            item.pet_end_name = buf.str()
    
        if version > 4:
            item.pet_power_name = buf.str()
    
        item.seed_bg = buf.u8()
        item.seed_fg = buf.u8()
        item.tree_bg = buf.u8()
        item.tree_fg = buf.u8()

        item.seed_bg_color = (buf.u8(), buf.u8(), buf.u8(), buf.u8())
        item.seed_fg_color = (buf.u8(), buf.u8(), buf.u8(), buf.u8())
    
        item.seed1 = buf.u16()
        item.seed2 = buf.u16()
    
        item.grow_time = buf.u32()
    
        if version > 6:
            item.fx_flags = buf.u32()
            item.multi_anim1 = buf.str()
    
        if version > 7:
            item.overlay_texture_file = buf.str()
            item.multi_anim2 = buf.str()
            item.dual_anim_layer = (buf.i32(), buf.i32())
    
        if version > 8:
            item.flags2 = buf.u32()
            item.client_data = [buf.i32() for _ in range(15)]
    
        if version > 9:
            item.tile_range = buf.u32()
            item.pile_size = buf.u32()
    
        if version > 10:
            item.customized_punch_parameters = buf.str()
    
        if version > 11:
            item.extra_slot_counter = buf.u32()
            item.extra_slot_body_parts = [buf.u8() for _ in range(9)]
    
        if version > 12:
            item.light_source_range = buf.u32()
    
        if version > 13:
            item.variant_version_item = buf.u32()
    
        if version > 14:
            item.chair_info.enabled = buf.u8()
            item.chair_info.player_offset = (buf.i32(), buf.i32())
            item.chair_info.arm_pos = (buf.i32(), buf.i32())
            item.chair_info.arm_offset = (buf.i32(), buf.i32())
            item.chair_info.arm_texture = buf.str()
    
        if version > 15:
            item.config_name = buf.str()
    
        if version > 16:
            item.other_player_hit_particle = buf.i32()
    
        if version > 17:
            item.config_name_hash = buf.u32()
    
        if version > 18:
            item.random_sprite_info.enabled = buf.u8()
            item.random_sprite_info.offset_mod = buf.i32()
            item.random_sprite_info.chance = buf.i32()
    
        if version > 19:
            item.hidden_parts_flags = buf.u8()
    
        if version > 20:
            item.can_transform = buf.u8()

        if version > 21:
            item.description = buf.str()

        if version > 22:
            item.seed1 = buf.u16()
            item.seed2 = buf.u16()
    
        if version > 23:
            item.slippery_type = buf.u8()
    
        if version > 24:
            _ = buf.str()
            _ = buf.u32()
    
        if version > 25:
            _ = buf.u8()
    
        return item

class ItemInfoManager:
    def __init__(self):
        self.items = {}
        self.version = 0

    def load(self, data):
        buf = Buffer(data)
    
        self.version = buf.u16()
        item_count = buf.u32()
    
        for _ in range(item_count):
            item = ItemInfo.deserialize(buf, self.version)
            self.items[item.id] = item

    def load_from_file(self, file_path: str):
        with open(file_path, "rb") as f:
            data = f.read()
            self.load(data)

    def get_item_by_id(self, item_id):
        if item_id >= len(self.items):
            return None

        return self.items.get(item_id)
    
    def get_by_name(self, name: str):
        name_lower = name.lower()
        for item in self.items.values():
            if item.name.lower() == name_lower:
                return item
        return None