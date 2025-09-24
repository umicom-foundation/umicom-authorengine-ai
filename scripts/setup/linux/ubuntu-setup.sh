#!/usr/bin/env bash
set -euo pipefail
apt update
apt -y upgrade
apt install -y build-essential clang lld lldb cmake ninja-build pkg-config git curl unzip zip \
               python3 python3-pip nodejs npm openjdk-21-jdk sqlite3
apt install -y docker.io || true
apt install -y meson libgtk-4-dev libadwaita-1-dev
echo "Ubuntu setup complete."
