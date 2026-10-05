#!/bin/bash
# ─── Smart Agriculture Build Script ────────────────────────────────────────
set -e

PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"

echo "═══════════════════════════════════════════════════"
echo "  Smart Agriculture Monitoring System — Build"
echo "═══════════════════════════════════════════════════"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

echo "[1/2] Running CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "[2/2] Compiling..."
make -j$(nproc)

echo ""
echo "✓ Build complete."
echo "  Binary: ${BUILD_DIR}/smart_agriculture"
echo ""
echo "  Run:  ./build/smart_agriculture --mode simulation"
echo "═══════════════════════════════════════════════════"
