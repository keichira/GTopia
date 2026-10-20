<div align="center">

# GTopia

### A cross-platform **Growtopia private server** with sub-server support

[![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-green.svg?style=for-the-badge)](#)
[![License](https://img.shields.io/badge/license-MIT-orange.svg?style=for-the-badge)](LICENSE)
[![Discord](https://img.shields.io/badge/Discord-Join%20us-5865F2?style=for-the-badge&logo=discord&logoColor=white)](https://discord.gg/5XjTQm3kRh)

</div>

---

## ✨ Features

| | Feature | Description |
| :-: | :-- | :-- |
| 🖥️ | **Cross-platform** | Runs on Windows, Linux and macOS. |
| 🧩 | **Sub-servers** | Split your game across multiple servers. |
| 🕰️ | **Multi-version** | Supports 0.92+ and the latest clients, **at the same time**. |
| 🛡️ | **Admin server** | Manage your server remotely. |
| 📜 | **Lua scripting** | Optional. Custom commands, dialogs and more. |
| 🗺️ | **World renderer** | Optional world map rendering. |

---

## 🚀 Quick Start
> The short version. For details on every step and OS, see the **[Setup & Running guide](Docs/Setup.md)**.

### 1. Install the requirements


**Linux (Ubuntu/Debian)**
```bash
sudo apt update
sudo apt install -y build-essential cmake python3 libmysqlclient-dev go
```

**Windows** – install these:
- [CMake](https://cmake.org/download/)
- [MySQL Community Server](https://dev.mysql.com/downloads/installer/)
- [Python 3](https://www.python.org/downloads/)
- [Go](https://go.dev/dl/)

### 2. Get the code
```bash
git clone https://github.com/keichira/GTopia.git
cd GTopia
```

### 3. Set up and build

All scripts are in the `Build/` folder.

| Step    | Windows         | Linux              |
| :------ | :-------------- | :----------------- |
| Setup   | `setup_win.bat` | `setup_linux.sh`   |
| Compile | `compile_win.bat` | `compile_linux.sh` |

### 4. Run

Open the `Runtime/` folder and start the launcher:

| Windows          | Linux               |
| :--------------- | :------------------ |
| `run_servers.bat` | `./run_servers.sh` |

> ⚠️ The HTTP server uses ports 80/443, so run the launcher as **administrator** or **root**.

---

## 📚 Documentation

Guides live in the [`Docs/`](Docs/) folder. Start with the **[Documentation Index](Docs/README.md)**.

---

## 🧩 How it works

- **Master Server** – handles player logins and manages all game servers and world renderer server.
- **Game Server** – runs the gameplay. Players reach it through the Master Server only. **[Benchmark.md](Docs/Benchmark.md)**.
- **World Renderer** – draws worlds using data from the Master Server and database.

---

## 📝 Notes

- `Runtime/` and `Build/` are protected by `.gitignore` whitelist rules, so your local configs (`config.txt`, `servers.txt`, logs) are never committed. Only the launcher scripts (`.sh`, `.bat`) are.
- **This project is for educational purposes only. The author is not responsible for any misuse. Use at your own risk.**

---

[GTopia](https://github.com/keichira/GTopia) is made by [keichira](https://github.com/keichira)