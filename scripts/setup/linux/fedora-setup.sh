#!/usr/bin/env bash
set -euo pipefail
dnf -y update
dnf -y groupinstall "Development Tools"
dnf -y install clang lld lldb cmake ninja-build pkg-config git curl unzip zip python3 python3-pip nodejs \
               java-21-openjdk java-21-openjdk-devel sqlite
dnf -y install meson gtk4-devel libadwaita-devel
dnf -y install docker || true
echo "Fedora setup complete."
