@echo off
title GTopia Launcher
cls

cd /d "%~dp0"

setlocal enabledelayedexpansion

SET GAME_SERVER_COUNT=0

if exist servers.txt (
    for /f "tokens=4 delims=|" %%a in ('findstr /r "^add_server|" servers.txt') do (
        set /a GAME_SERVER_COUNT+=%%a
    )
)

if %GAME_SERVER_COUNT% equ 0 (
    SET GAME_SERVER_COUNT=1
)

endlocal & set "FINAL_SERVER_COUNT=%GAME_SERVER_COUNT%"

echo Detected Game Server Count from config: %FINAL_SERVER_COUNT%

echo Launching HTTPS Server...
start "GTopia - HTTPS Server" cmd /c "cd ..\HTTPServer && go run main.go"
timeout /t 3 /nobreak >nul

echo Launching Master Server...
start "GTopia - Master Server" .\Master.exe
timeout /t 3 /nobreak >nul

echo Launching %FINAL_SERVER_COUNT% Game Server Instances...
for /L %%i in (1,1,%FINAL_SERVER_COUNT%) do (
    echo Spawning Game Server ID: %%i
    start "GTopia - Game Server ID %%i" .\GameServer.exe --id %%i
    timeout /t 1 /nobreak >nul
)

echo ==================================================
echo All instances running in background!
pause