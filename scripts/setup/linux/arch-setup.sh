#!/usr/bin/env bash
set -euo pipefail
pacman -Syu --noconfirm
pacman -S --noconfirm base-devel clang lld lldb cmake ninja pkgconf git curl unzip zip python python-pip nodejs npm \
                      jdk21-openjdk sqlite
pacman -S --noconfirm meson gtk4 libadwaita
pacman -S --noconfirm docker || true
echo "Arch setup complete."
