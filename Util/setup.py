import os
import sys
import glob
import shutil
import platform
import subprocess
import urllib.request
import socket
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import urlparse

def print_success(msg): print(f"✅ {msg}")
def print_error(msg): print(f"❌ {msg}")
def print_warn(msg): print(f"⚠️ {msg}")
def print_info(msg): print(f"ℹ️ {msg}")

PROJECT_ROOT = Path(__file__).resolve().parent.parent

CERT_DIR = (PROJECT_ROOT / "HTTPServer").resolve()
RUNTIME_DIR = (PROJECT_ROOT / "Runtime").resolve()
CONFIGS_DIR = (PROJECT_ROOT / "Configs").resolve()
SQL_FILE = (CONFIGS_DIR / "gtopia.sql").resolve()
SQL_UPDATES_DIR = (CONFIGS_DIR / "Updates").resolve()

sys.path.append(str(PROJECT_ROOT / "Util"))

try:
    from update_file_hashes import generate_file_hashes
    from generate_item_data import generate_item_txt_from_dat
    from generate_wiki_data import fetch_wiki_and_write
except ImportError as e:
    print_error(f"Required helper script missing or broken: {e}")
    print_info("Please ensure you cloned the repository completely.")
    sys.exit(1)

MKCERT_VERSION = "v1.4.4"
MKCERT_BASE = f"https://github.com/FiloSottile/mkcert/releases/download/{MKCERT_VERSION}"

@dataclass
class DatabaseConfig:
    name: str = "gtopia"
    user: str = "root"
    password: str = ""

def run_silent(cmd):
    return subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

def run_powershell(command: str):
    try:
        result = subprocess.run(
            ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", command],
            capture_output=True, text=True, check=True
        )

        return True, result.stdout.strip()
    except subprocess.CalledProcessError as e:
        return False, (e.stderr or e.stdout or str(e)).strip()

def download_file(url, path: Path) -> bool:
    try:
        print_info(f"Downloading: {url}")
        urllib.request.urlretrieve(url, path)
        return True
    except Exception as e:
        print_error(f"Download failed: {e}")
        return False

def get_local_ip() -> str:
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(("8.8.8.8", 80))
            return s.getsockname()[0]
    except Exception:
        return "127.0.0.1"

def get_clean_input(prompt_text, default=None) -> str:
    suffix = f" [{default}]" if default is not None else ""
    user_input = input(f"{prompt_text}{suffix}: ").strip().strip('"').strip("'")
    return default if not user_input and default is not None else user_input

def get_valid_path(prompt_text, is_file=False, is_dir=False) -> Path:
    while True:
        path_str = get_clean_input(prompt_text)
        if not path_str:
            print_warn("Path cannot be empty.")
            continue
            
        path = Path(path_str).expanduser().resolve()
        if is_file and not path.is_file():
            print_error(f"Target is not a valid file: {path}")
        elif is_dir and not path.is_dir():
            print_error(f"Target is not a valid directory: {path}")
        else:
            return path

        if get_clean_input("👉 Type 'S' to skip this step, or press Enter to try again", default="").lower() == 's':
            return None

def move_file(src, dst_dir) -> bool:
    src, dst_dir = Path(src), Path(dst_dir)
    if not src.exists():
        print_error(f"File not found: {src.name}")
        return False

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

def check_cmake():
    print_info("Checking CMake installation...")
    if shutil.which("cmake"):
        print_success("CMake is ready.")
        return

    print_error("CMake was not found on your system PATH.")
    if platform.system() == "Windows":
        print_info("Please install CMake from: https://cmake.org/download/")
        print_warn("CRITICAL: Remember to check 'Add CMake to the system PATH' during installation!")
    else:
        print_info("Install via your package manager: sudo apt install cmake")
    sys.exit(1)

def check_mysql() -> str:
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
    else:
        print_info("Install via package manager: sudo apt install mysql-client libmysql-dev")
    sys.exit(1)

def task_generate_items():
    print("\n--- [Item Data Generator] ---")
    dat_path = get_valid_path("Enter your raw items.dat path location", is_file=True)
    if dat_path:
        print_info("Generating item data...")
        generate_item_txt_from_dat(0, dat_path)
        move_file(PROJECT_ROOT / "items.txt", RUNTIME_DIR)

def task_generate_wiki():
    print("\n--- [Wiki Data Generator] ---")
    wiki_dat_path = get_valid_path("Enter your raw items.dat path location", is_file=True)
    if wiki_dat_path:
        print_info("Processing wiki data, it might take a bit...")
        fetch_wiki_and_write(0, wiki_dat_path)
        move_file(PROJECT_ROOT / "wiki_data.txt", RUNTIME_DIR)

def task_generate_hashes():
    print("\n--- [File Hash Generator] ---")
    print_info("Enter the static folder containing 'audio', 'interface', and 'game' subdirectories.")
    static_path = get_valid_path("Enter Static folder path", is_dir=True)
    if static_path:
        print_info("Processing file hashes, it might take a bit...")
        generate_file_hashes(static_path)
        move_file(PROJECT_ROOT / "filehashes.txt", RUNTIME_DIR)

def get_mkcert_url() -> str:
    sys_type = platform.system().lower()
    arch = platform.machine().lower()
    
    os_info = {
        "windows": {"amd64": "windows-amd64.exe"},
        "linux": {"x86_64": "linux-amd64", "amd64": "linux-amd64", "aarch64": "linux-arm64", "arm64": "linux-arm64"},
        "darwin": {"arm64": "darwin-arm64", "amd64": "darwin-amd64"}
    }
    
    try:
        suffix = os_info[sys_type][arch] or os_info[sys_type]["amd64"]
        return f"{MKCERT_BASE}/mkcert-{MKCERT_VERSION}-{suffix}"
    except KeyError:
        raise Exception(f"Unsupported platform: {sys_type}-{arch}")

def setup_ssl_certificates():
    print_info("Initializing local environment trusted SSL generation...")
    exe_name = "mkcert.exe" if platform.system() == "Windows" else "mkcert"
    mkcert_path = PROJECT_ROOT / exe_name

    if not mkcert_path.exists():
        if not download_file(get_mkcert_url(), mkcert_path):
            print_error("Failed to download mkcert.")
            return
        if platform.system() != "Windows":
            os.chmod(str(mkcert_path), 0o755)

    print_info("Generating root certificates (You may see an UAC)...")
    run_silent([str(mkcert_path), "-install"])
    run_silent([str(mkcert_path), "*.growtopia1.com", "*.growtopia2.com"])
    
    CERT_DIR.mkdir(parents=True, exist_ok=True)
    for target_pem in glob.glob("*.pem"):
        dest_name = "key.pem" if "-key" in os.path.basename(target_pem) else "cert.pem"
        dest_path = CERT_DIR / dest_name
        if dest_path.exists():
            os.remove(dest_path)
        shutil.move(target_pem, str(dest_path))
        print_success(f"Deployed local certificate: {dest_name}")

def get_latest_growtopia_cdn() -> str:
    print_info("Fetching latest Ubisoft CDN...")
    url = "https://growtopiagame.com/Growtopia-Installer.exe"
    opener = urllib.request.build_opener(urllib.request.HTTPRedirectHandler())
    try:
        res = opener.open(urllib.request.Request(url))
        final_url = res.geturl()
    except Exception as e:
        print_error(f"Connection error while fetching CDN layout: {e}")
        return ""

    if "akamaihd.net" not in final_url:
        return ""

    parsed = urlparse(final_url)
    cdn_pathname = parsed.path.replace("GrowtopiaInstaller.exe", "")
    return f"{parsed.hostname}{cdn_pathname}cache/"

def run_mysql_query(mysql_client, args, sql_input):
    try:
        return subprocess.run(
            [mysql_client] + args, input=sql_input, text=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE
        )
    except Exception as e:
        print_error(f"MySQL process bridge crash: {e}")
        return None

def get_existing_databases(mysql_client, user: str, password: str) -> list:
    auth_args = ["-u", user]
    if password:
        auth_args.append(f"-p{password}")
        
    res = run_mysql_query(mysql_client, auth_args, "SHOW DATABASES;")
    if res and res.returncode == 0 and res.stdout:
        system_dbs = {"information_schema", "performance_schema", "mysql", "sys"}
        dbs = [
            line.strip() for line in res.stdout.strip().splitlines()[1:] 
            if line.strip() and line.strip().lower() not in system_dbs
        ]
        return dbs
    return []

def init_schema_migrations(mysql_client, auth_args, db_name):
    sql = f"""
    USE `{db_name}`;
    CREATE TABLE IF NOT EXISTS `SchemaMigrations` (
      `Version` VARCHAR(255) PRIMARY KEY,
      `ApplyTime` TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    ) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
    """
    run_mysql_query(mysql_client, auth_args, sql)

def update_database_tables(mysql_client, cfg: DatabaseConfig):
    auth_args = ["-u", cfg.user]
    if cfg.password:
        auth_args.append(f"-p{cfg.password}")

    run_mysql_query(mysql_client, auth_args, f"CREATE DATABASE IF NOT EXISTS `{cfg.name}`;")
    init_schema_migrations(mysql_client, auth_args, cfg.name)

    get_versions_sql = f"USE `{cfg.name}`; SELECT `Version` FROM `SchemaMigrations`;"
    res = run_mysql_query(mysql_client, auth_args, get_versions_sql)
    
    applied_versions = set()
    if res and res.returncode == 0 and res.stdout:
        applied_versions = set(line.strip() for line in res.stdout.strip().splitlines()[1:] if line.strip())

    if not SQL_UPDATES_DIR.exists():
        SQL_UPDATES_DIR.mkdir(parents=True, exist_ok=True)

    all_updates = sorted([f for f in SQL_UPDATES_DIR.glob("*.sql")])
    pending_updates = [f for f in all_updates if f.name not in applied_versions]

    if not pending_updates:
        return True, 0, []

    applied_now = []
    for sql_file in pending_updates:
        print_info(f"Applying migration: {sql_file.name}...")

        sql_content = sql_file.read_text(encoding="utf-8")
        import_res = run_mysql_query(mysql_client, auth_args + [cfg.name], sql_content)
        
        if import_res and import_res.returncode == 0:
            record_sql = f"USE `{cfg.name}`; INSERT INTO `SchemaMigrations` (`Version`) VALUES ('{sql_file.name}');"
            run_mysql_query(mysql_client, auth_args, record_sql)
            applied_now.append(sql_file.name)
            print_success(f"Applied: {sql_file.name}")
        else:
            err = import_res.stderr if import_res else "Unknown Error"
            print_error(f"Failed to apply {sql_file.name}:\n{err}")
            return False, len(applied_now), applied_now

    return True, len(applied_now), applied_now

def run_update_sql_tables():
    print("\n--- [Update SQL Tables] ---")
    mysql_client = check_mysql()
    
    user = get_clean_input("MySQL username", default="root")
    password = get_clean_input("MySQL password", default="")

    print_info("Fetching existing databases...")
    dbs = get_existing_databases(mysql_client, user, password)
    
    if dbs:
        print_info(f"Found existing database(s): {', '.join(dbs)}")
        default_db = dbs[0] if "gtopia" not in dbs else "gtopia"
    else:
        print_warn("No custom databases found or unable to list databases.")
        default_db = "gtopia"

    db_name = get_clean_input("Database name to update", default=default_db)    
    cfg = DatabaseConfig(name=db_name, user=user, password=password)
    
    print_info(f"Checking for updates in '{SQL_UPDATES_DIR.name}' folder...")
    success, count, applied = update_database_tables(mysql_client, cfg)
    
    if success:
        if count > 0:
            print_success(f"Successfully applied {count} migration(s): {', '.join(applied)}")
        else:
            print_info("Database is already up to date. No pending migrations.")
    else:
        print_error("Failed to execute one or more database migrations.")

def run_database_wizard(mysql_client) -> DatabaseConfig:
    print("\n--- MySQL Configuration Database Wizard ---")
    while True:
        cfg = DatabaseConfig(
            name=get_clean_input("Database name to use", default="gtopia"),
            user=get_clean_input("MySQL username", default="root"),
            password=get_clean_input("MySQL password", default="")
        )
        
        print_info(f"Connecting to MySQL and initializing scheme '{cfg.name}'...")
        sql_init = f"CREATE DATABASE IF NOT EXISTS {cfg.name};"
        auth_args = ["-u", cfg.user] if not cfg.password else ["-u", cfg.user, f"-p{cfg.password}"]
        
        result = run_mysql_query(mysql_client, auth_args, sql_init)
        if not result or result.returncode != 0:
            err_msg = result.stderr if result else "Process Failure"
            print_error(f"Failed to connect or create schema. MySQL Error:\n{err_msg}")
            if get_clean_input("👉 Re-enter credentials? [Y/n]", default="y").lower() == 'y':
                continue
            return None
            
        print_success(f"Database schema '{cfg.name}' validated.")
        if not SQL_FILE.exists():
            print_error(f"Target SQL file missing at: {SQL_FILE}")
            return cfg
            
        print_info("Importing tables...")
        import_result = run_mysql_query(mysql_client, auth_args + [cfg.name], SQL_FILE.read_text(encoding="utf-8"))
        if import_result and import_result.returncode == 0:
            print_success(f"Integrated tables into target '{cfg.name}'.")
            return cfg
        else:
            print_error(f"Table configuration failed: {import_result.stderr if import_result else 'Internal Error'}")
            return cfg

def update_config_line(file_path: Path, prefix: str, replacement: str):
    if not file_path.exists():
        return
    lines = file_path.read_text(encoding="utf-8").splitlines(keepends=True)
    for i, line in enumerate(lines):
        if line.strip().startswith(prefix):
            lines[i] = replacement
            break
    file_path.write_text("".join(lines), encoding="utf-8")

def configure_environment_firewalls(server_count: int):
    rules = [
        {"name": "GTopia-UDP-GamePorts", "proto": "UDP", "ports": f"18000-{18000 + server_count}"},
        {"name": "GTopia-TCP-SocketPorts", "proto": "TCP", "ports": f"18500-{18500 + server_count}"}
    ]
    print_info("Creating Firewall inbound rules on host shell...")
    for rule in rules:
        ps_script = f'''
        Remove-NetFirewallRule -DisplayName "{rule['name']}" -ErrorAction SilentlyContinue
        New-NetFirewallRule -DisplayName "{rule['name']}" -Direction Inbound -Action Allow -Protocol {rule['proto']} -LocalPort "{rule['ports']}"
        '''
        ok, output = run_powershell(ps_script)
        if ok:
            print_success(f"Deployed Windows Firewall Rule: {rule['name']} -> {rule['proto']} ({rule['ports']})")
        else:
            print_error(f"Failed to deploy rule: {rule['name']}\n{output}")

def edit_configuration_files(db: DatabaseConfig, local_ip: str, latest_cdn: str):
    print("\n--------------------------------------------------")
    print("         Automated Configuration Editor           ")
    print("--------------------------------------------------")
    if get_clean_input("👉 Automate configurations directly into text files? [Y/n]", default="y").lower() != "y":
        print_info("Skipping automation edits. You will need to customize configuration manually.")
        return

    print("\nSelect:")
    print("1) Local (LAN)")
    print("2) Virtual Private Server (VPS/VDS)")

    choice = get_clean_input("Select configuration profile [1-2]", default="1")
    wan_ip = local_ip if choice == "1" else get_clean_input("Enter Public WAN IP Address")
    lan_ip = local_ip if choice == "2" else "127.0.0.1"

    print_info("Applying target changes...")

    cdn_host, cdn_path = latest_cdn.split("/", 1) if "/" in latest_cdn else ("", "")
    update_config_line(RUNTIME_DIR / "config.txt", "database_info|", f"database_info|localhost|{db.user}|{db.password}|{db.name}|3306|\n")
    if latest_cdn:
        update_config_line(RUNTIME_DIR / "config.txt", "cdn_server|", f"cdn_server|{cdn_host}|{cdn_path}|\n")
    
    (RUNTIME_DIR / "worlds").mkdir(parents=True, exist_ok=True)
    update_config_line(RUNTIME_DIR / "config.txt", "world_save_path|", f"world_save_path|{RUNTIME_DIR / 'worlds'}|\n")
    
    update_config_line(RUNTIME_DIR / "servers.txt", "set_master|", f"set_master|{lan_ip}|{wan_ip}|\n")
    update_config_line(RUNTIME_DIR / "servers.txt", "add_server|", f"add_server|{lan_ip}|{wan_ip}|1|\n")
    update_config_line(RUNTIME_DIR / "telnet_config.txt", "telnet_host|", f"telnet_host|{lan_ip}|\n")
    update_config_line(CERT_DIR / "main.go", "const SERVER_IP =", f'const SERVER_IP = "{wan_ip}"\n')

    print_success("Configuration done.")

    if platform.system() == "Windows" and wan_ip != local_ip:
        if get_clean_input("\n👉 Automatically create Windows Firewall paths? [Y/n]", default="y").lower() == "y":
            try:
                count = int(get_clean_input("Expected game server count?", default="1"))
                configure_environment_firewalls(count)
            except ValueError:
                print_error("Invalid parameter provided.")

def run_full_setup():
    print("==========================================")
    print("      GTopia Private Server Setup Wizard  ")
    print("==========================================")
    print_info("Community: https://discord.gg/5XjTQm3kRh\n")

    check_cmake()
    mysql_client = check_mysql()
    
    db_config = DatabaseConfig()
    if get_clean_input("\n👉 Initialize database and import tables? [Y/n]", default="y").lower() == "y":
        wizard_result = run_database_wizard(mysql_client)
        if wizard_result:
            db_config = wizard_result

    if get_clean_input("\n👉 Generate local environment root SSL certificates? (Required for V3.90+) [Y/n]", default="y").lower() == "y":
        setup_ssl_certificates()

    RUNTIME_DIR.mkdir(parents=True, exist_ok=True)

    if get_clean_input("\n👉 Generate filehashes.txt? (Skip if not managing a custom CDN) [y/N]", default="n").lower() == "y":
        task_generate_hashes()

    if get_clean_input("\n👉 Generate items.txt? (Required) [Y/n]", default="y").lower() == "y":
        task_generate_items()

    if get_clean_input("\n👉 Generate wiki_data.txt from wiki? [y/N]", default="n").lower() == "y":
        task_generate_wiki()

    print("\n--- Moving configs ---")
    config_files = [
        "config.txt", "playmods.txt", "roles.txt", "telnet_config.txt", 
        "servers.txt", "achievements.txt", "store.txt", "consumable_data.txt", "battle_pet_data.txt"
    ]
    for config in config_files:
        move_file(CONFIGS_DIR / config, RUNTIME_DIR)

    (RUNTIME_DIR / "logs").mkdir(parents=True, exist_ok=True)
    (RUNTIME_DIR / "logs" / "crash").mkdir(parents=True, exist_ok=True)
    
    local_ip = get_local_ip()
    latest_cdn = get_latest_growtopia_cdn()
    edit_configuration_files(db_config, local_ip, latest_cdn)

    print("\n==========================================")
    print("          SETUP COMPLETED SUCCESSFULLY    ")
    print("==========================================\n")
    print_info(f"Detected LAN IP: {local_ip}")
    if latest_cdn:
        print_info(f"Latest Ubisoft CDN: {latest_cdn}")

def run_get_lan_ip():
    print("\n--- [LAN IP Info] ---")
    print_info(f"Your detected Local LAN IP is: {get_local_ip()}")
    print_warn("Shown LAN IP address might be wrong if you are using proxy, run `ifconfig` or `ipconfig` to see all interfaces")
    print_warn("If you are running on a remote VPS/VDS, use your public server IP instead.\n")

def run_get_latest_cdn():
    print("\n--- [Latest Ubisoft CDN] ---")
    cdn = get_latest_growtopia_cdn()
    if cdn:
        print_info(f"Active Link Found: {cdn}")
    else:
        print_error("Failed to fetch latest Ubisoft CDN.")

def main():
    menu_actions = {
        "1": run_full_setup,
        "2": run_get_latest_cdn,
        "3": run_get_lan_ip,
        "4": task_generate_items,
        "5": task_generate_wiki,
        "6": task_generate_hashes,
        "7": run_update_sql_tables
    }

    while True:
        subprocess.run("cls" if platform.system() == "Windows" else "clear", shell=True)

        print("\n==================================================")
        print("       GTOPIA PRIVATE SERVER SETUP PANEL          ")
        print("==================================================")
        print(" [1] Start Full Server Setup Wizard")
        print(" [2] Fetch Latest Ubisoft CDN")
        print(" [3] Get LAN IP")
        print(" [4] Generate items.txt")
        print(" [5] Generate wiki_data.txt")
        print(" [6] Generate filehashes.txt")
        print(" [7] Update SQL Tables")
        print(" [0] Exit")
        print_info("Community: https://discord.gg/5XjTQm3kRh")

        choice = get_clean_input("Choice", default="1")
        if choice == "0":
            print_info("Exiting setup. Bye!")
            break
        
        action = menu_actions.get(choice)
        if action:
            action()
            print("\n--------------------------------------------------")
            input("👉 Press Enter to return to the Main Menu...")
        else:
            print_error("Invalid option selected.")
            input("👉 Press Enter to try again...")

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print_error("\nSetup processes interrupted.")
        sys.exit(0)