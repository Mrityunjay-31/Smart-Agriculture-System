#!/bin/bash
# ─── Smart Agriculture Linux Setup Script ──────────────────────────────────
# Tested on: Ubuntu 22.04 / Debian 12
set -e

echo "═══════════════════════════════════════════════════"
echo "  Smart Agriculture — Linux Dependency Setup"
echo "═══════════════════════════════════════════════════"

echo "[1/5] Updating package lists..."
sudo apt-get update -y

echo "[2/5] Installing build tools..."
sudo apt-get install -y \
    build-essential \
    cmake \
    g++ \
    git

echo "[3/5] Installing libraries..."
sudo apt-get install -y \
    libcurl4-openssl-dev \
    libasio-dev \
    libgtest-dev \
    zlib1g-dev

echo "[4/5] Installing serial port tools..."
sudo apt-get install -y \
    minicom \
    screen

echo "[5/5] Adding user to dialout group (serial port access)..."
sudo usermod -aG dialout "$USER"

echo ""
echo "═══════════════════════════════════════════════════"
echo "  ✓ Setup complete."
echo ""
echo "  IMPORTANT: Log out and log back in for the"
echo "  dialout group change to take effect."
echo ""
echo "  Next steps:"
echo "    1. cp .env.example .env"
echo "    2. Edit .env with your Firebase credentials"
echo "    3. cp config/config.example.conf config/config.conf"
echo "    4. ./scripts/build.sh"
echo "    5. ./scripts/run.sh simulation"
echo "═══════════════════════════════════════════════════"
