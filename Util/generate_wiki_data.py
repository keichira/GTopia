import urllib.parse
import urllib.request
import json
import re
import time
import sys
from pathlib import Path
from item_info_manager import ItemInfoManager

BATCH_SIZE = 49
BASE_URL = "https://growtopia.fandom.com"

class ItemWikiData:
    def __init__(self):
        self.description = ""
        self.element = "NONE"

        self.seed1 = ""
        self.seed2 = ""

        self.combine1 = ""
        self.combine1Amount = ""
        self.combine2 = ""
        self.combine2Amount = ""
        self.combine3 = ""
        self.combine3Amount = ""
        self.combineResultAmount = "1"

def trim_braces(s: str) -> str:
    pos = s.find("}}")
    if pos != -1:
        return s[:pos]

    pos = s.find("}")
    if pos != -1:
        return s[:pos]

    return s


def extract_templates(data: str):
    return re.findall(r"\{\{.*?\}\}", data, re.DOTALL)

def split_args(template: str):
    return template.split("|")

def parse_item_wiki(data: str) -> ItemWikiData:
    out = ItemWikiData()

    templates = extract_templates(data)

    for tpl in templates:
        args = split_args(tpl)
        if not args:
            continue

        t = trim_braces(args[0])

        if t == "{{Item":
            if len(args) >= 2:
                out.description = trim_braces(args[1])

            if len(args) >= 3:
                out.element = trim_braces(args[2])

        elif t == "{{RecipeSplice":
            if len(args) >= 2:
                out.seed1 = args[1]
            if len(args) >= 3:
                out.seed2 = trim_braces(args[2])

        elif t == "{{RecipeCombine":
            if len(args) < 6:
                continue

            out.combine1 = args[1]
            out.combine1Amount = args[2]
            out.combine2 = args[3]
            out.combine2Amount = args[4]
            out.combine3 = args[5]

            if len(args) >= 7:
                out.combine3Amount = trim_braces(args[6])

            if len(args) >= 8:
                out.combineResultAmount = trim_braces(args[7])

    return out

def http_get(url: str) -> str:
    req = urllib.request.Request(
        url,
        headers={"User-Agent": "Mozilla/5.0"}
    )

    with urllib.request.urlopen(req) as resp:
        if resp.status != 200:
            return ""

        return resp.read().decode("utf-8", errors="ignore")

def fetch_wiki_and_write(serialize_until: int = 0, file_path: str = "items.dat"):
    mgr = ItemInfoManager()
    mgr.load_from_file(file_path)

    item_count = len(mgr.items)
    if serialize_until == 0 or serialize_until > item_count:
        serialize_until = item_count

    batches = []
    current = []

    for i in range(serialize_until):
        item = mgr.get_item_by_id(i)
        if item is None:
            continue

        if item.id % 2 != 0:
            continue

        if "null_item" in item.name:
            continue

        current.append(item.name)
        if len(current) == 49:
            batches.append("|".join(current))
            current = []

    if current:
        batches.append("|".join(current))

    print("Fetching wiki...")

    with open("wiki_data.txt", "w", encoding="utf-8") as f:
        for i, batch in enumerate(batches):
            query = (
                "/api.php?action=query"
                "&prop=revisions"
                "&rvprop=content"
                "&format=json"
                "&titles=" + urllib.parse.quote(batch)
            )

            url = BASE_URL + query

            body = http_get(url)
            if not body:
                print("Empty body:", batch)
                continue

            try:
                data = json.loads(body)
            except Exception:
                continue

            pages = data.get("query", {}).get("pages", {})

            for page_id, page_data in pages.items():

                item_name = page_data.get("title", "")
                revisions = page_data.get("revisions")
                if not revisions:
                    continue

                wiki_text = revisions[0].get("*", "")
                if not wiki_text:
                    continue

                item = mgr.get_by_name(item_name)
                if item is None:
                    continue

                wiki = parse_item_wiki(wiki_text)

                write = f"#{item_name}\n"
                write += f"set_wiki|{item.id}|"

                if not all((wiki.seed1, wiki.seed2)):
                    write += "0|0|"
                else:
                    seed_1 = mgr.get_by_name(wiki.seed1)
                    seed_2 = mgr.get_by_name(wiki.seed2)

                    if not all((seed_1, seed_2)):
                        continue

                    write += f"{seed_1.id + 1}|{seed_2.id + 1}|"

                write += f"{wiki.element.upper()}|"
                write += f"{wiki.description}|"

                if (
                    wiki.combine1
                    and wiki.combine2
                    and wiki.combine3
                ):
                    combine_1 = mgr.get_by_name(wiki.combine1)
                    combine_2 = mgr.get_by_name(wiki.combine2)
                    combine_3 = mgr.get_by_name(wiki.combine3)

                    if not all((combine_1, combine_2, combine_3)):
                        continue

                    write += "\n"
                    write += f"set_combine|{item.id}|"
                    write += f"{wiki.combine1Amount}|{combine_1.id}|"
                    write += f"{wiki.combine2Amount}|{combine_2.id}|"
                    write += f"{wiki.combine3Amount}|{combine_3.id}|"

                write += "\n\n"
                f.write(write)

            print(f"Parsed batch {i+1}/{len(batches)}")
            time.sleep(0.02)

    print("Fetch wiki finished")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python generate_wiki_data.py <items_dat_path>")
        sys.exit(1)

    path = Path(sys.argv[1]).expanduser().resolve()

    fetch_wiki_and_write(0, path)