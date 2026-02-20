@echo off
echo Select project to build:
echo 1) Master
echo 2) GameServer
set /p project_choice=Project: 

if "%project_choice%"=="1" set PROJECT=Master
if "%project_choice%"=="2" set PROJECT=GameServer
if not defined PROJECT (
    echo Invalid project type
    exit /b 1
)

echo.
echo Select build mode:
echo 1) Debug
echo 2) Release
set /p mode_choice=Build Mode: 

if "%mode_choice%"=="1" set MODE=Debug
if "%mode_choice%"=="2" set MODE=Release
if not defined MODE (
    echo Invalid build mode
    exit /b 1
)

set BUILD_DIR=build_%PROJECT%
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"
cd /d "%BUILD_DIR%"

echo Building %PROJECT% (%MODE%)
cmake ../../CMake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%MODE% -DSERVER_TO_BUILD=%PROJECT%
cmake --build .

echo.
echo %PROJECT% build completed!
pause