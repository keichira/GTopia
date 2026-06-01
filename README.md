# GTopia

[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-green.svg)](#)
[![License](https://img.shields.io/badge/license-MIT-orange.svg)](LICENSE)

A cross-platform **Growtopia private server** featuring sub-server support.

> 📢 **Join our community!** For development updates, support, and discussions, join our **[Discord Server](https://discord.gg/5XjTQm3kRh)**.

---

## 🌟 Features

*   **Cross-Platform:** Full compatibility with both Linux and Windows environments.
*   **Sub-Server Support:** Split your game instances across sub-servers.
*   **Telnet Management:** Built-in telnet interface for remote server administration.
*   **Multi-version** supports 3.02+ and latest versions
*   **World Renderer Support:** Submodule inclusion for world map rendering.

---

## 🧩 Architecture

### 🧠 Master Server
The Master Server is responsible for managing all Servers and handling player login requests. It acts as the central coordinator and routes players to Game Servers.

### 🎮 Game Server
Game Servers handle actual gameplay logic. They only communicate with the Master Server and do not accept direct external connections from clients.

### 🖼️ World Renderer
World Renderers are responsible for visualizing the game world received from the Master Server. They only communicate with the Master Server and do not handle gameplay or client logic.

---

## 🛠️ Prerequisites

Before cloning and building, ensure you have the following environment dependencies installed based on your Operating System.

### 🐧 Linux (Ubuntu/Debian)
```bash
sudo apt update
sudo apt install -y build-essential cmake python3 mysql-client libmysql-dev go
```

### 🪟 Windows
- [CMake](https://cmake.org/download/)
- [MySQL Community Server](https://dev.mysql.com/downloads/installer/)
- [Python 3](https://www.python.org/downloads/) – Used for scripts and automation
- [Go](https://go.dev/dl/) – Used for HTTP/S server

---

## 🚀 Getting Started
### Clone the Repository
```bash
git clone https://github.com/keichira/GTopia.git
cd GTopia
```

## ⚙️ Setup & Build System

This project uses pre-configured scripts for setup and compilation.  
All builds are managed through platform-specific scripts.

* All build and setup scripts are located inside the `Build/` directory.

### 🔨 Interactive Setup Scripts

- `setup_win.bat` → Installs/configures requires on Windows
- `setup_linux.sh` → Installs/configures requires on Linux

### 🛠️ Compile Scripts

- `compile_win.bat` → Builds selected project on Windows
- `compile_linux.sh` → Builds selected project on Linux

---

## 🏃‍♂️ Running the Servers
Once compiled, navigate to the `Runtime/` folder.

> ⚠️ Because the HTTP server binds to privileged ports (80/443), you **must** execute the launcher with root/administrative privileges

### 🐧 Linux.

* **Start all servers:** Run `./run_servers.sh`
* **Stop all servers:** Run `./kill_servers.sh`

### 🪟 Windows

* **Start all servers:** Double-click `run_servers.bat`
* **Stop all servers:** Double-click `kill_servers.bat`

---

## Note
> The `Runtime/` directory is guarded by whitelist rules via `.gitignore`. Your localized configs (`config.txt`, `servers.txt`, logs) will never be accidentally leaked to the repository. Only the automation launchers (`.sh`, `.bat`) are committed.

**This project is for educational purposes only. The author is not responsible for any misuse. Use at your own risk.**

---

<a href="https://github.com/keichira/GTopia">GTopia</a> is made by <a href="https://github.com/keichira">keichira</a>