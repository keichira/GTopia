import os
import sys
import glob
import shutil
import platform
import subprocess
import urllib.request
from pathlib import Path
import socket
from update_file_hashes import generate_file_hashes
from generate_item_data import generate_item_txt_from_dat

ROOT = Path.cwd()
CERT_DIR = (ROOT / ".." / "HTTPServer").resolve()
RUNTIME_DIR = (ROOT / ".." / "Runtime").resolve()
CONFIGS_DIR = (ROOT / ".." / "Configs").resolve()

MKCERT_VERSION = "v1.4.4"
MKCERT_BASE = f"https://github.com/FiloSottile/mkcert/releases/download/{MKCERT_VERSION}"

MYSQL_PATHS = [
    r"C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe",
    r"C:\Program Files (x86)\MySQL\MySQL Server 8.0\bin\mysql.exe",
]
SQL_FILE = os.path.join(ROOT, "..", "Configs", "gtopia.sql")
    
def run(cmd, check=True):
    return subprocess.run(cmd, shell=isinstance(cmd, str), check=check)

def run_silent(cmd):
    return subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def download_file(url, path: Path):
    try:
        print(f"Downloading: {url}")
        urllib.request.urlretrieve(url, path)
        return True

    except Exception as e:
        print(f"Download failed: {e}")

    return False

def get_mkcert_url():
    sys = platform.system().lower()
    arch = platform.machine().lower()

    if sys == "windows":
        return f"{MKCERT_BASE}/mkcert-{MKCERT_VERSION}-windows-amd64.exe"

    if sys == "linux":
        if arch in ["x86_64", "amd64"]:
            return f"{MKCERT_BASE}/mkcert-{MKCERT_VERSION}-linux-amd64"
        if arch in ["aarch64", "arm64"]:
            return f"{MKCERT_BASE}/mkcert-{MKCERT_VERSION}-linux-arm64"

    if sys == "darwin":
        if arch in ["arm64"]:
            return f"{MKCERT_BASE}/mkcert-{MKCERT_VERSION}-darwin-arm64"
        return f"{MKCERT_BASE}/mkcert-{MKCERT_VERSION}-darwin-amd64"

    raise Exception(f"Unsupported platform: {sys}-{arch}")

def move_file(src, dst):
    if not os.path.exists(src):
        print(f"❌ File not found: {src}")
        return False

    os.makedirs(os.path.dirname(dst), exist_ok=True)

    if dst.exists():
        print(f"⚠️ Skipped (already exists): {dst}")
        return False

    shutil.move(src, dst)
    print(f"✅ Moved: {src} -> {dst}")
    return True

def move_certificates():
    os.makedirs(CERT_DIR, exist_ok=True)

    for f in glob.glob("*.pem"):
        try:
            base = os.path.basename(f)

            if "-key" in base:
                dest_name = "key.pem"
            else:
                dest_name = "cert.pem"

            dest = os.path.join(CERT_DIR, dest_name)

            if os.path.exists(dest):
                os.remove(dest)

            shutil.move(f, dest)
            print(f"✅ Moved: {base} -> {dest_name}")

        except Exception as e:
            print(f"❌ Failed moving {f}: {e}")

def download_mkcert():
    print("Downloading mkcert for certificate generation...")

    exe_name = "mkcert.exe" if platform.system() == "Windows" else "mkcert"
    mkcert_path = os.path.join(ROOT, exe_name)

    if not os.path.exists(mkcert_path):
        ok = download_file(get_mkcert_url(), mkcert_path)
        if not ok:
            raise Exception("mkcert download failed")

        if platform.system() != "Windows":
            os.chmod(mkcert_path, 0o755)

    print("mkcert installed generating certificates...")
    run_silent([f"{mkcert_path}", "-install"])
    run_silent([f"{mkcert_path}", "*.growtopia1.com", "*.growtopia2.com"])
    print("Generated certificates")
    move_certificates()

def open_install_page(url):
    print("\nOpening web page...")

    try:
        import webbrowser
        webbrowser.open(url)
    except:
        print("Please open manually:", url)

def check_cmake():
    print("Checking CMake")

    if not shutil.which("cmake"):
        print("❌ CMake not found")
        
        if platform.system() == "Windows":
            print("Install: https://cmake.org/download/")
            ans = input("\n👉 Do you want to open installer page now? [y/N]: ").strip().lower()

            if ans == "y":
                open_install_page("https://cmake.org/download/")
        else:
            print("\nInstall CMake using your package manager:")
            print("Ubuntu/Debian: sudo apt install cmake")
        sys.exit(1)

    print("✅ CMake OK")

def check_mysql():
    print("Checking MySQL")

    if platform.system() == "Windows":
        for path in MYSQL_PATHS:
            if os.path.exists(path):
                print("✅ MySQL OK")
                return path

    mysql = shutil.which("mysql")

    if mysql:
        print("✅ MySQL OK")
        return mysql

    print("❌ MySQL not found")
    if platform.system() == "Windows":
        print("Install: https://dev.mysql.com/downloads/installer/ (not web version)")
        print("Make sure 'MySQL Server' + 'Add to PATH' is enabled")

        ans = input("\n👉 Do you want to open installer page now? [y/N]: ").strip().lower()

        if ans == "y":
            open_install_page("https://dev.mysql.com/downloads/installer/")
    else:
        print("\nInstall MySQL using your package manager:")
        print("Ubuntu/Debian: sudo apt install mysql-client libmysql-dev")
    sys.exit(1)

def run_mysql(mysql_client, args, sql):
    try:
        return subprocess.run(
            [mysql_client] + args,
            input=sql,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
    
    except Exception as e:
        print(f"❌ MySQL command failed: {e}")
        return None

def sql_create_database(mysql_client, db_name, user, password):
    print(f"Creating database {db_name} if not exists...")
    sql = f"CREATE DATABASE IF NOT EXISTS {db_name};"

    result = run_mysql(
        mysql_client,
        ["-u", user, f"-p{password}"],
        sql
    )

    if result and result.returncode == 0:
        print(f"✅ Database {db_name} ready")
        return True
    
    print(f"❌ Failed to create database {db_name}")
    return False

def sql_import_tables(mysql_client, db_name, user, password):
    print("Importing SQL file...")

    if not os.path.exists(SQL_FILE):
        print(f"❌ SQL file not found: {SQL_FILE}")
        return False

    with open(SQL_FILE, "r", encoding="utf-8") as f:
        sql = f.read()

    result = run_mysql(
        mysql_client,
        ["-u", user, f"-p{password}", db_name],
        sql
    )

    if result:
        print(f"✅ Imported SQL tables to {db_name}")
        return True

    print(f"❌ Failed to import SQL tables to {db_name}")
    return False

def sql_wizard(mysql_client):
    db_name = input("Database name [gtopia]: ").strip() or "gtopia"
    user = input("MySQL user [root]: ").strip() or "root"
    password = input("MySQL password []: ").strip()
    
    print("\n--- Starting SQL Setup ---")
    if not sql_create_database(mysql_client, db_name, user, password):
        return False
    
    if not sql_import_tables(mysql_client, db_name, user, password):
        return False
    
    return True

def get_local_ip():
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.connect(("8.8.8.8", 80))
    ip = s.getsockname()[0]
    s.close()
    return ip

def file_hashes_wizard():
    print("\n--- Starting file hash generation ---")

    print("\nWhat is static folder path?: The folder that contains 'audio', 'interface', 'game' folders")
    print("Used for generating file hashes for items.dat\n")

    folder_path = input("Static folder path: ").strip()
    path = Path(folder_path).expanduser().resolve()

    print("Generating filehashes.txt this might took a bit")
    generate_file_hashes(path)

def item_txt_wizard():
    print("\n--- Starting item data generation ---")

    folder_path = input("items.dat file path: ").strip()
    path = Path(folder_path).expanduser().resolve()

    print("Generating items.txt this might took a bit")
    generate_item_txt_from_dat(0, path)

def main():
    check_cmake()
    print("----------\n")

    mysql_client = check_mysql()

    ans = input("\n👉 Do you want to import SQL tables? [y/N]: ").strip().lower()
    if ans == "y":
        result = sql_wizard(mysql_client)
        if not result:
            print("❌ Failed to import SQL tables, skipping...")
    print("----------\n")

    ans = input("\n👉 Do you want to create SSL Certificates? [y/N]: ").strip().lower()
    if ans == "y":
        download_mkcert()
    print("----------\n")

    os.makedirs(RUNTIME_DIR, exist_ok=True)

    ans = input("\n👉 Do you want to generate file hashes? [y/N]: ").strip().lower()
    if ans == "y":
        file_hashes_wizard()
        print("----------\n")
        move_file(ROOT / "filehashes.txt", RUNTIME_DIR)
    else:
        print("Okay! you can generate by using update_file_hashes.py later\n")

    ans = input("\n👉 Do you want to generate items.txt [y/N]: ").strip().lower()
    if ans == "y":
        item_txt_wizard()
        print("----------\n")
        move_file(ROOT / "items.txt", RUNTIME_DIR)
    else:
        print("Okay! you can generate by using generate_item_data.py later\n")

    move_file(CONFIGS_DIR / "config.txt", RUNTIME_DIR)
    move_file(CONFIGS_DIR / "playmods.txt", RUNTIME_DIR)
    move_file(CONFIGS_DIR / "roles.txt", RUNTIME_DIR)
    move_file(CONFIGS_DIR / "telnet_config.txt", RUNTIME_DIR)
    move_file(CONFIGS_DIR / "servers.txt", RUNTIME_DIR)
    move_file(CONFIGS_DIR / "achievements.txt", RUNTIME_DIR)
    move_file(CONFIGS_DIR / "store.txt", RUNTIME_DIR)
        
    print("\nSetup finished\n")
    print(f"Your LAN IP address is `{get_local_ip()}`, if you are not running on VPS/VDS use this IP address to host")
    print("Shown LAN IP address might be wrong if you are using proxy, run `ifconfig` or `ipconfig` to see all interfaces\n")
    print("\nNow navigate to Runtime folder and edit configs")

if __name__ == "__main__":
    main()