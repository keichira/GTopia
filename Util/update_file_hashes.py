import os
import sys

def hash(data: bytes) -> int:
    h = 0x55555555

    for c in data:
        h = (((h >> 27) & 0x1F) + ((h << 5) & 0xFFFFFFFF) + c) &  0xFFFFFFFF

    return h

if len(sys.argv) < 2:
    print("Usage: python3 update_file_hashes.py <folder_path>")
    exit(1)
else:
    baseFolder = sys.argv[1]

if not os.path.isdir(baseFolder):
    print(f"{baseFolder} is not exists")
    exit(1)

output = open("filehashes.txt", "w")

exludedFolders = {}
includedExt = {".rttex", ".wav", ".ogg", ".mp3"}

for(root, dirs, files) in os.walk(baseFolder):
    dirs[:] = [d for d in dirs if d not in exludedFolders]

    for file in files:
        ext = os.path.splitext(file)[1]
        if not ext.lower() in {e.lower() for e in includedExt}:
            continue
        
        fullPath = os.path.join(root, file)
        with open(fullPath, "rb") as data:
            bytes = data.read()
            relativePath = os.path.relpath(fullPath, baseFolder)
            output.write(
                relativePath.replace("\\", "/") + "|" +
                str(hash(bytes)) + "|\n"
            )
            
output.close()