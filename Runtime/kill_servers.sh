#!/bin/bash

if [ "$EUID" -ne 0 ]; then
  echo "Plase run this script with 'sudo ./kill_servers.sh'"
  exit 1
fi

echo "Terminating all background instances..."

pkill -f "go run main.go"
pkill -f Master
pkill -f GameServer

echo "=================================================="
echo "All background instances terminated successfully."
