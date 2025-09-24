#!/usr/bin/env bash
set -euo pipefail
sudo apt update
sudo apt -y upgrade
sudo apt install -y build-essential clang lld lldb cmake ninja-build pkg-config git curl unzip zip
sudo apt install -y meson libgtk-4-dev libadwaita-1-dev
sudo apt install -y libssl-dev zlib1g-dev libsqlite3-dev
echo "Done."
