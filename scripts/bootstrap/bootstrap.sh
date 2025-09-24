#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# PURPOSE: Project helper script.
#
# Created by: Umicom Foundation (https://umicom.foundation/)
# Developer: Sammy Hegab
# Date: 24-09-2025
# License: MIT
# -----------------------------------------------------------------------------
# NOTE:
# Every line is commented so even a beginner can follow.
# -----------------------------------------------------------------------------

set -euo pipefail

# Detect OS type using uname
OS="$(uname -s || true)"

case "$OS" in
  Darwin)
    echo "[bootstrap] macOS detected. Running Homebrew setup..."
    bash "$(dirname "$0")/../setup/macos/brew-setup.sh"
    ;;
  Linux)
    echo "[bootstrap] Linux detected. Routing to distro bootstrap..."
    sudo bash "$(dirname "$0")/../setup/linux/bootstrap.sh"
    ;;
  *)
    echo "[bootstrap] Unsupported OS: $OS"
    exit 1
    ;;
esac

echo "[bootstrap] Done. Build with: cmake --preset default && cmake --build build -j"
