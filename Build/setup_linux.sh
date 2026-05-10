set -e

if ! command -v python3 &> /dev/null
then
    echo "Python3 not found! Install python3."
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

python3 "$ROOT_DIR/Util/setup.py"