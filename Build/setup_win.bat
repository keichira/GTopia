@echo off

python --version >nul 2>&1
if %errorlevel% neq 0 (
    echo Python not found! Install Python.
    pause
    exit /b 1
)

for %%I in ("%~dp0..") do set ROOT_DIR=%%~fI
python "%ROOT_DIR%\Util\setup.py"