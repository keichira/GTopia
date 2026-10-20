# Items & Wiki Data Documentation

This guide provides an in-depth overview of managing item data using `items.txt`, configuring client-side item serialization, adding custom items, and generating `wiki_data.txt`.

---

## Overview of `items.txt`

To simplify item management. Allowing server administrators to view, edit, add, or remove items easily—the server utilizes `items.txt` instead of the raw binary `items.dat`.

* **Supported `items.dat` Versions:** `0` through `27`.

### How to Generate `items.txt`
1. Run the **Setup Script**.
2. Select option **`[4] Generate items.txt`**.
3. Provide the path to your `items.dat` file (or drag and drop the file into the terminal).
4. Move the newly generated `items.txt` file into the **`Runtime/`** directory.

---

## Server Configuration (`Runtime/config.txt`)

The server pre-serializes item data in memory to match target client versions. Configure these options inside `Runtime/config.txt`:

```text
force_item_data_version|0

android_supported_versions|0.02|5.58
windows_supported_versions|0.02|5.58
ios_supported_versions|0.02|5.58
macos_supported_versions|0.02|5.58
```

### Key Field Breakdown

#### `force_item_data_version`
* **`0` (Disabled):** The server auto-detects each player's client version and sends the pre-serialized item data.
* **`1` to `26`:** Forces a specific item data version for all connected clients regardless of their version.

#### Version Ranges & Memory Usage
The server iterates through the version span between the calculated **minVersion** and **maxVersion** across all platforms (`X_supported_versions`):

$$\text{Version Span} = [\text{minVersion}, \text{maxVersion})$$

> **⚠️ Memory Impact Note:** 
> A wider range between minimum and maximum supported versions means the server pre-serializes and stores more versions of the item database in memory simultaneously. **Keeping your supported version range narrower reduces RAM allocation.**

---

## ZLib Compression & Optimization Rules

Clients with version **`>= 3.02`** support ZLib compression for item data, significantly reducing network bandwidth and memory footprint.

### Logic

```cpp
bool shouldCompress = allSupportZLib || (m_version == 0 && i >= 12);
```

* **Full ZLib Optimization:**
  If all minimum platform versions in `config.txt` are set to `3.02` or higher (e.g., `3.02|5.58`), **all** serialized item data blocks are compressed using ZLib.
* **Fallback / Mixed Version Support:**
  If any minimum platform version drops below `3.02` (e.g., `2.59`), ZLib compression is only applied to **item data versions $\ge 12$**. Versions below $12$ remain uncompressed.

---

## Custom Items

To add custom server-side items without conflicting with official item IDs, use **negative item IDs**.

### Guidelines
1. **Negative IDs:** Assign unique negative integers for custom items (e.g., `-1`, `-3`, `-5`).
2. **Custom Asset Delivery:** Custom items require custom texture assets served via a custom CDN. See [HTTP.md](HTTP.md) for CDN setup details.
3. **Packet & Dialog Encoding:**
   When sending custom item IDs inside dialogs (`embed_data`), raw packets, or packet structures, always process the ID through `ToItemClientID`:
   ```cpp
   int16 clientItemId = ToItemClientID(customItemId);
   ```
4. **Seed Conversion Utility:**
   When converting between custom seeds and items, always use the dedicated conversion helpers:
   * `SEED_TO_ITEM_ID(seedId)`
   * `ITEM_TO_SEED_ID(itemId)`

---

## Wiki Data (`wiki_data.txt`)

`wiki_data.txt` contains item metadata sourced from the official Growtopia Fandom Wiki. It is used by the server engine to handle:
* **Seed Splicing Recipes**
* **Item Combining Recipes**
* **Item Descriptions**
* **Item Elements**

### How to Generate `wiki_data.txt`
1. Run the **Setup Script**.
2. Select option **`[5] Generate wiki_data.txt`**.
3. Provide the path to your `items.dat` file (or drag and drop it into the terminal).
4. Move the generated `wiki_data.txt` file into the **`Runtime/`** directory.