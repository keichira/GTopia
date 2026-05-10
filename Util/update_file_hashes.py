import os
import sys

def hash(data: bytes) -> int:
    h = 0x55555555

    for c in data:
        h = (((h >> 27) & 0x1F) + ((h << 5) & 0xFFFFFFFF) + c) &  0xFFFFFFFF

    return h

def generate_file_hashes(base_folder: str, output_file: str = "filehashes.txt"):
    if not os.path.isdir(base_folder):
        raise ValueError(f"{base_folder} is not a valid directory")

    excluded_folders = set()
    included_ext = {".rttex", ".wav", ".ogg", ".mp3"}

    with open(output_file, "w", encoding="utf-8") as output:
        for root, dirs, files in os.walk(base_folder):
            dirs[:] = [d for d in dirs if d not in excluded_folders]

            for file in files:
                ext = os.path.splitext(file)[1]
                if ext.lower() not in included_ext:
                    continue

                full_path = os.path.join(root, file)

                with open(full_path, "rb") as f:
                    data_bytes = f.read()

                relative_path = os.path.relpath(full_path, base_folder)

                output.write(
                    relative_path.replace("\\", "/") + "|" +
                    str(hash(data_bytes)) + "|\n"
                )


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python update_file_hashes.py <folder_path> [output_file]")
        sys.exit(1)

    base_folder = sys.argv[1]

    output_file = "filehashes.txt"
    if len(sys.argv) >= 3:
        output_file = sys.argv[2]

    generate_file_hashes(base_folder, output_file)