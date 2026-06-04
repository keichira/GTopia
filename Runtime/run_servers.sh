#!/bin/bash

if [ "$EUID" -ne 0 ]; then
  echo "Please run this script with 'sudo ./run_servers.sh'"
  exit 1
fi

if [ -f "servers.txt" ]; then
    GAME_SERVER_COUNT=$(awk -F'|' '/^[[:space:]]*add_server\|/ {sum += $4} END {print sum}' servers.txt)
else
    GAME_SERVER_COUNT=1
fi

if [ -z "$GAME_SERVER_COUNT" ] || [ "$GAME_SERVER_COUNT" -eq 0 ]; then
    GAME_SERVER_COUNT=1
fi
echo "Detected Game Server Count from config: $GAME_SERVER_COUNT"

echo "Launching HTTPS Server..."
(cd ../HTTPServer && go run main.go) &
sleep 2

echo "Launching Master Server..."
./Master &
sleep 3

echo "Launching $GAME_SERVER_COUNT Game Server Instances..."
for ((i=1; i<=GAME_SERVER_COUNT; i++))
do
    echo "Spawning Game Server ID: $i"
    ./GameServer --id $i &
    sleep 1
done

echo "=================================================="
echo "All instances running in background!"
