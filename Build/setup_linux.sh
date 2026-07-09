#!/bin/bash
set -e

if ! command -v python3 &> /dev/null; then
    echo "Python3 not found! Please install python3."
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

cd "$ROOT_DIR"

python3 -m Util.setup