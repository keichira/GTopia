@echo off

python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Python not found! Please install Python.
    pause
    exit /b 1
)

for %%I in ("%~dp0..") do set "PROJECT_ROOT=%%~fI"
cd /d "%PROJECT_ROOT%"

python -m Util.setup

pause