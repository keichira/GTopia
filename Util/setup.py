import os
import sys
import glob
import shutil
import platform
import subprocess
import urllib.request
from urllib.parse import urlparse
from pathlib import Path
import socket

def print_success(msg): print(f"✅ {msg}")
def print_error(msg): print(f"❌ {msg}")
def print_warn(msg): print(f"⚠️ {msg}")
def print_info(msg): print(f"ℹ️ {msg}")

try:
    from update_file_hashes import generate_file_hashes
    from generate_item_data import generate_item_txt_from_dat
    from generate_wiki_data import fetch_wiki_and_write
except ImportError as e:
    print_error(f"Required helper script missing or broken: {e}")
    print_info("Please ensure you cloned the repository completely.")
    sys.exit(1)

ROOT = Path(__file__).resolve().parent
CERT_DIR = (ROOT / ".." / "HTTPServer").resolve()
RUNTIME_DIR = (ROOT / ".." / "Runtime").resolve()
CONFIGS_DIR = (ROOT / ".." / "Configs").resolve()

MKCERT_VERSION = "v1.4.4"
MKCERT_BASE = f"https://github.com/FiloSottile/mkcert/releases/download/{MKCERT_VERSION}"
SQL_FILE = (ROOT / ".." / "Configs" / "gtopia.sql").resolve()
    
def run_silent(cmd):
    return subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def download_file(url, path: Path):
    try:
        print_info(f"Downloading: {url}")
        urllib.request.urlretrieve(url, path)
        return True

    except Exception as e:
        print_error(f"Download failed: {e}")
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

def move_file(src, dst_dir):
    src = Path(src)
    if not src.exists():
        print_error(f"File not found: {src.name}")
        return False

    dst_dir = Path(dst_dir)
    dst_dir.mkdir(parents=True, exist_ok=True)
    dst = dst_dir / src.name

    if dst.exists():
        print_warn(f"Skipped (already exists): {dst.name}")
        return False

    try:
        shutil.move(str(src), str(dst))
        print_success(f"Moved: {src.name} -> {dst_dir.name}/")
        return True
    except Exception as e:
        print_error(f"Failed to move {src.name}: {e}")
        return False
    
def get_clean_input(prompt_text, default=None):
    suffix = f" [{default}]" if default else ""
    user_input = input(f"{prompt_text}{suffix}: ").strip()
    
    #drag
    user_input = user_input.strip('"').strip("'")
    
    if not user_input and default is not None:
        return default
    return user_input

def get_valid_path(prompt_text, is_file=False, is_dir=False):
    while True:
        path_str = get_clean_input(prompt_text)
        if not path_str:
            print_warn("Path cannot be empty.")
            continue
            
        path = Path(path_str).expanduser().resolve()
        
        if is_file and not path.is_file():
            print_error(f"Target is not a valid file or doesn't exist: {path}")
            ans = input("👉 Type 'S' to skip this step, or press Enter to try again: ").strip().lower()
            if ans == 's': return None
            continue
            
        if is_dir and not path.is_dir():
            print_error(f"Target is not a valid directory or doesn't exist: {path}")
            ans = input("👉 Type 'S' to skip this step, or press Enter to try again: ").strip().lower()
            if ans == 's': return None
            continue
            
        return path

def check_cmake():
    print_info("Checking CMake installation...")

    if not shutil.which("cmake"):
        print_error("CMake was not found on your system PATH.")

        if platform.system() == "Windows":
            print_info("Please install CMake from: https://cmake.org/download/")
            print_warn("CRITICAL: Remember to check 'Add CMake to the system PATH' during installation!")
        else:
            print_info("Install via your package manager:\nUbuntu/Debian: sudo apt install cmake")
        sys.exit(1)

    print_success("CMake is ready.")

def check_mysql():
    print_info("Checking MySQL installation...")
    
    if platform.system() == "Windows":
        possible_patterns = [
            r"C:\Program Files\MySQL\MySQL Server *\bin\mysql.exe",
            r"C:\Program Files (x86)\MySQL\MySQL Server *\bin\mysql.exe"
        ]
        for pattern in possible_patterns:
            matches = glob.glob(pattern)
            if matches:
                detected_path = matches[-1]
                print_success(f"MySQL detected automatically at: {detected_path}")
                return detected_path

    mysql = shutil.which("mysql")
    if mysql:
        print_success(f"MySQL detected on system PATH: {mysql}")
        return mysql

    print_error("MySQL client executable could not be found.")
    if platform.system() == "Windows":
        print_info("Download: https://dev.mysql.com/downloads/installer/")
        print_warn("Ensure 'MySQL Server' is checked and added to system PATH variables.")
    else:
        print_info("Install via package manager:\nUbuntu/Debian: sudo apt install mysql-client libmysql-dev")
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
        print_error(f"MySQL bridge crash: {e}")
        return None

def sql_wizard(mysql_client):
    print("\n--- MySQL Configuration Database Wizard ---")
    
    while True:
        db_name = get_clean_input("Database name to use/create", default="gtopia")
        user = get_clean_input("MySQL username", default="root")
        password = get_clean_input("MySQL password", default="")
        
        print_info(f"Connecting to MySQL and preparing database '{db_name}'...")
        
        sql = f"CREATE DATABASE IF NOT EXISTS {db_name};"
        result = run_mysql(mysql_client, ["-u", user, f"-p{password}"], sql)
        
        if not result or result.returncode != 0:
            err_msg = result.stderr if result else "Unknown process error"
            print_error(f"Failed to connect or create database. MySQL Error:\n{err_msg}")

            retry = get_clean_input("👉 Would you like to re-enter your credentials? [Y/n]", default="y").lower()
            if retry == 'y':
                continue
            return False
            
        print_success(f"Database '{db_name}' validated safely.")
        
        if not SQL_FILE.exists():
            print_error(f"SQL file missing at: {SQL_FILE}")
            return False
            
        with open(SQL_FILE, "r", encoding="utf-8") as f:
            sql_data = f.read()
            
        print_info("Importing SQL tables...")
        import_res = run_mysql(mysql_client, ["-u", user, f"-p{password}", db_name], sql_data)
        
        if import_res and import_res.returncode == 0:
            print_success(f"Successfully integrated all tables into '{db_name}'.")
            return True
        else:
            print_error(f"Table scheme import failed: {import_res.stderr if import_res else 'Process communication error'}")
            return False
        
def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except:
        return ""

def download_mkcert():
    print_info("Initializing local environment trusted SSL generation...")
    exe_name = "mkcert.exe" if platform.system() == "Windows" else "mkcert"
    mkcert_path = ROOT / exe_name

    if not mkcert_path.exists():
        if not download_file(get_mkcert_url(), mkcert_path):
            print_error("Failed to automatically grab mkcert deployment asset.")
            return
        if platform.system() != "Windows":
            os.chmod(str(mkcert_path), 0o755)

    print_info("Generating root certificates (You might see an UAC prompt)...")
    run_silent([str(mkcert_path), "-install"])
    run_silent([str(mkcert_path), "*.growtopia1.com", "*.growtopia2.com"])
    
    CERT_DIR.mkdir(parents=True, exist_ok=True)
    for f in glob.glob("*.pem"):
        base = os.path.basename(f)
        dest_name = "key.pem" if "-key" in base else "cert.pem"
        dest_path = CERT_DIR / dest_name

        if dest_path.exists():
            os.remove(dest_path)

        shutil.move(f, str(dest_path))
        print_success(f"Deployed local cert asset: {dest_name}")

def get_latest_growtopia_cdn():
    url = "https://growtopiagame.com/Growtopia-Installer.exe"

    req = urllib.request.Request(url)
    opener = urllib.request.build_opener(urllib.request.HTTPRedirectHandler())

    try:
        res = opener.open(req)
        final_url = res.geturl()
    except Exception as e:
        print_error(f"Connection error while getting latest cdn {e}")
        return ""

    if "akamaihd.net" not in final_url:
        return ""

    parsed = urlparse(final_url)
    gt_host = parsed.hostname    
    gt_pathname = parsed.path.replace("GrowtopiaInstaller.exe", "")

    return f"{gt_host}{gt_pathname}cache/"

def edit_configuration_files(db_name, user, password, local_ip, latest_cdn):
    print("\n--------------------------------------------------")
    print("       Automated Configuration Editor            ")
    print("--------------------------------------------------")
    
    ans = get_clean_input("👉 Do you want to automatically edit your config files? [Y/n]", default="y").lower()
    if ans != "y":
        print_info("Skipping auto-edit. You will need to edit .txt files manually.")
        return

    config_path = RUNTIME_DIR / "config.txt"
    telnet_path = RUNTIME_DIR / "telnet_config.txt"
    servers_path = RUNTIME_DIR / "servers.txt"
    http_path = (ROOT / ".." / "HTTPServer" / "main.go").resolve()

    print("\nSelect your hosting environment:")
    print("1) Local PC / LAN")
    print("2) VPS / VDS)")
    choice = get_clean_input("Select option [1-2]", default="1")

    target_wan_ip = local_ip if local_ip else "127.0.0.1"
    target_lan_ip = local_ip if local_ip else "127.0.0.1"

    if choice == "2":
        target_wan_ip = get_clean_input("Enter your Public VPS/VDS IP address")
        target_lan_ip = local_ip

    print_info("Applying updates to runtime files...")

    if config_path.exists():
        with open(config_path, "r", encoding="utf-8") as f:
            lines = f.readlines()
        for i, line in enumerate(lines):
            if line.startswith("database_info|"):
                lines[i] = f"database_info|localhost|{user}|{password}|{db_name}|3306|\n"

            elif line.startswith("cdn_server|") and latest_cdn:
                if "/" in latest_cdn:
                    parts = latest_cdn.split("/", 1)
                    cdn_host = parts[0]
                    cdn_path = parts[1]
                    lines[i] = f"cdn_server|{cdn_host}|{cdn_path}|\n"

            elif line.startswith("world_save_path|"):
                (RUNTIME_DIR / "worlds").mkdir(parents=True, exist_ok=True)
                lines[i] = f"world_save_path|{str(RUNTIME_DIR / 'worlds')}|\n"

        with open(config_path, "w", encoding="utf-8") as f:
            f.writelines(lines)

        print_success("config.txt -> Updated database and CDN links.")

    if servers_path.exists():
        with open(servers_path, "r", encoding="utf-8") as f:
            lines = f.readlines()
        for i, line in enumerate(lines):
            if line.startswith("set_master|"):
                lines[i] = f"set_master|{target_lan_ip}|{target_wan_ip}|\n"

            elif line.startswith("add_server|"):
                lines[i] = f"add_server|{target_lan_ip}|{target_wan_ip}|1|\n"

        with open(servers_path, "w", encoding="utf-8") as f:
            f.writelines(lines)
        print_success("servers.txt -> Updated servers.")

    if telnet_path.exists():
        with open(telnet_path, "r", encoding="utf-8") as f:
            lines = f.readlines()
        for i, line in enumerate(lines):
            if line.startswith("telnet_host|"):
                lines[i] = f"telnet_host|{target_lan_ip}|\n"

        with open(telnet_path, "w", encoding="utf-8") as f:
            f.writelines(lines)
        print_success("telnet_config.txt -> Updated admin server.")

    if http_path.exists():
        with open(http_path, "r", encoding="utf-8") as f:
            http_lines = f.readlines()
            
        for i, line in enumerate(http_lines):
            if line.strip().startswith('const SERVER_IP ='):
                http_lines[i] = f'const SERVER_IP = "{target_wan_ip}"\n'
                
        with open(http_path, "w", encoding="utf-8") as f:
            f.writelines(http_lines)
        print_success("main.go -> Updated SERVER_IP.")

    print_success("All configurations edited successfully!\n")

def main():
    print("==========================================")
    print("      GTopia Private Server Setup Wizard   ")
    print("==========================================")
    print_info("Discord server: https://discord.gg/5XjTQm3kRh\n")

    check_cmake()
    mysql_client = check_mysql()

    db_name, db_user, db_pass = "gtopia", "root", ""

    if get_clean_input("\n👉 Do you want to import SQL tables? [Y/n]", default="y").lower() == "y":
        sql_wizard(mysql_client)

    if get_clean_input("\n👉 Do you want to create local SSL Certificates for HTTPS server? (It needed for 3.90+ versions) [Y/n]", default="y").lower() == "y":
        download_mkcert()

    RUNTIME_DIR.mkdir(parents=True, exist_ok=True)

    if get_clean_input("\n👉 Do you want to generate file hashes? (Skip if not using custom CDN) [y/N]", default="n").lower() == "y":
        print_info("\nEnter the static folder containing 'audio', 'interface', 'game' subdirectories.")
        static_path = get_valid_path("Enter Static folder path", is_dir=True)
        if static_path:
            print_info("Processing file hashes, it might take a bit...")
            generate_file_hashes(static_path)
            move_file(ROOT / "filehashes.txt", RUNTIME_DIR)

    if get_clean_input("\n👉 Do you want to generate items.txt from your items.dat? (Required) [Y/n]", default="y").lower() == "y":
        dat_path = get_valid_path("Enter your raw items.dat path location", is_file=True)
        if dat_path:
            print_info("Processing items...")
            generate_item_txt_from_dat(0, dat_path)
            move_file(ROOT / "items.txt", RUNTIME_DIR)

    if get_clean_input("\n👉 Do you want to generate wiki_data.txt? (Description/Splice information) (Not required) [y/N]", default="n").lower() == "y":
        wiki_dat_path = get_valid_path("Enter your raw items.dat path location", is_file=True)
        if wiki_dat_path:
            print_info("Processing wiki data, it might take a bit...")
            fetch_wiki_and_write(0, wiki_dat_path)
            move_file(ROOT / "wiki_data.txt", RUNTIME_DIR)

    print("\n--- Moving default configurations ---")
    config_files = [
        "config.txt", "playmods.txt", "roles.txt", "telnet_config.txt", 
        "servers.txt", "achievements.txt", "store.txt", "consumable_data.txt", "battle_pet_data.txt"
    ]
    for config in config_files:
        move_file(CONFIGS_DIR / config, RUNTIME_DIR)

    (RUNTIME_DIR / "logs").mkdir(parents=True, exist_ok=True)

    print_info("Getting latest Growtopia CDN...")
    latest_cdn = get_latest_growtopia_cdn()

    edit_configuration_files(db_name, db_user, db_pass, get_local_ip(), latest_cdn)

    print("\n==========================================")
    print("          SETUP COMPLETED SUCCESSFULLY    ")
    print("==========================================\n")
    print_info(f"Your detected Local LAN IP is: {get_local_ip()}")
    print_warn("Shown LAN IP address might be wrong if you are using proxy, run `ifconfig` or `ipconfig` to see all interfaces")
    print_warn("If you are running on a remote VPS/VDS, use your public server IP instead.\n")

    if latest_cdn:
        print_info(f"Latest Growtopia CDN: {latest_cdn}")

    print_success("Navigate to the 'Runtime' folder and customize your configurations.\n")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print_error("\nSetup cancelled by user.")
        sys.exit(0)