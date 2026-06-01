@echo off
title GTopia Stopper
cls

echo Terminating all background instances...

taskkill /IM GameServer.exe
taskkill /IM Master.exe

taskkill /FI "WINDOWTITLE eq GTopia - HTTPS Server*" /IM cmd.exe

echo ==================================================
echo All background instances terminated successfully.
pause