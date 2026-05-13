# Growtopia Private Server
Cross-platform Growtopia private server with sub-server support

New Discord server for development: https://discord.gg/5XjTQm3kRh

## Clone
Clone the repo : ``git clone --recurse-submodules https://github.com/keichira/GTopia.git``<br>
*if you don't want to run **WorldRenderer** remove ``--recurse-submodules``*

## Setup
Project requires (will be asked in setup script)

- [CMake](https://cmake.org/download/)
- [MySQL Community](https://dev.mysql.com/downloads/installer/)<br>

#### Linux
*Go to **Build** folder* and run
```
chmod +x setup_linux.sh
./setup_linux.sh
```

#### Windows
*Go to **Build** folder* and run
```
setup_win.bat
```

## Configure
After running setup navigate to **Runtime** folder and edit given fields below. You can use given LAN IP from setup script for host or VPS/VDS (given more info in setup)

- config.txt (examples given in config.txt)
```
database_info|
world_save_path|
cdn_server|
```

- servers.txt (examples given in servers.txt)
```
set_master|
add_server|
```

- telnet_config.txt (examples given in telnet_config.txt)
```
telnet_host|
```

- items.txt <br>
Game servers are not working with raw items.dat you have to generate it into txt format, you can use generate_item_data script to generate it (it will be asked in setup script)
<br>

- wiki_data.txt <br>
No usage of it right now

## Compile
- *Scripts are gonna ask you what you want to build*<br>

#### Linux
*Go to **Build** folder* and run
```
chmod +x compile_linux.sh
./compile_linux.sh
```

#### Windows
*Go to **Build** folder* and run
```
compile_linux.bat
```

# Note
This tool is for educational purposes only. The author is not responsible for any misuse. Use at your own risk.
<hr>
<a href="https://github.com/keichira/GTopia">GTopia</a> is made by <a href="https://github.com/keichira">keichira</a>
