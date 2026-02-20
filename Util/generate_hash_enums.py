import os
import sys

def hash(s: str) -> int:
    h = 0x12668559
    
    for c in s.encode():
        h ^= c
        h = (h * 0x5bd1e995) & 0xFFFFFFFF
        h ^= (h >> 15)
        h &= 0xFFFFFFFF

    return h

if len(sys.argv) < 2:
    print("Usage: python3 generate_hash_enums.py <file_path>")
    exit(1)
else:
    filePath = sys.argv[1]

if not os.path.isfile(filePath):
    print(f"{filePath} is not exists")
    exit(1)

with open(filePath) as f:
    keys = [line.strip() for line in f if line.strip()]

col = {}
prefix = ""
replaceConds = []

for k in keys:
    if(k.startswith("prefix|")):
        prefix = k.split("|")[1]
        continue
    elif(k.startswith("replace|")):
        repArgs = k.split("|")
        replaceConds.append([repArgs[1], repArgs[2]])
        continue

    hashed_str = hash(k)
    if hashed_str in col:
        print(f"Hash collision {k} - {col[hashed_str]}")

    col[hashed_str] = k

    outStr = k
    for old, new in replaceConds:
        outStr = outStr.replace(old, new)

    print(f"{prefix}{outStr.upper()} = {hash(k)}u,")