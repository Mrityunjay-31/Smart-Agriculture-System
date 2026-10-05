#!/bin/bash
# ─── Smart Agriculture Run Script ──────────────────────────────────────────
set -e

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BINARY="${PROJECT_DIR}/build/smart_agriculture"

if [ ! -f "${BINARY}" ]; then
    echo "ERROR: Binary not found. Run scripts/build.sh first."
    exit 1
fi

echo "═══════════════════════════════════════════════════"
echo "  Smart Agriculture Monitoring System"
echo "═══════════════════════════════════════════════════"

# Default arguments
MODE="${1:-simulation}"
DEVICE="${2:-/dev/ttyACM0}"
BAUD="${3:-9600}"
PORT="${4:-8080}"

echo "  Mode:   ${MODE}"
echo "  Device: ${DEVICE}"
echo "  Baud:   ${BAUD}"
echo "  Port:   ${PORT}"
echo "═══════════════════════════════════════════════════"

exec "${BINARY}" \
    --mode "${MODE}" \
    --device "${DEVICE}" \
    --baud "${BAUD}" \
    --port "${PORT}"
