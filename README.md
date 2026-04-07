# Growtopia Private Server
Cross-platform Growtopia private server

## Project Info
**Master**: Managing servers and logging players<br>
**GameServer**: Running the game instance<br>
**ItemManager**: Creating or managing items for server<br>
**WorldRenderer**: Rending worlds and saving as png<br>

## Cloning
Clone the repo : ``git clone --recurse-submodules https://github.com/keichira/GTopia.git``<br>
*if you don't want to run **WorldRenderer** remove ``--recurse-submodules``*

## Building

### Linux
- Installation

*Depends on your distro*
```sh
sudo apt update
sudo apt install cmake libmysqlclient-dev
```

- Compile

*Go to Build folder* and run
```sh
chmod +x compile_linux.sh
./compile_linux.sh
```

### Windows
- Installation

Download [CMake](https://cmake.org/download/)<br>
Download MinGW ([MSYS2](https://www.msys2.org/))<br>
Download [MySQL Community](https://dev.mysql.com/downloads/installer/)

- Compile

*Go to Build folder* and run
```
compile_win.bat
```
