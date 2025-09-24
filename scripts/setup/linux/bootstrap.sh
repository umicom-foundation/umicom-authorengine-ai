#!/usr/bin/env bash
set -euo pipefail

ID=""
if [ -f /etc/os-release ]; then
  . /etc/os-release
  ID="${ID:-}"
fi

case "$ID" in
  ubuntu|debian)
    exec sudo bash "$(dirname "$0")/ubuntu-setup.sh"
    ;;
  fedora)
    exec sudo bash "$(dirname "$0")/fedora-setup.sh"
    ;;
  arch|endeavouros|manjaro)
    exec sudo bash "$(dirname "$0")/arch-setup.sh"
    ;;
  *)
    echo "Unsupported distro: $ID"
    echo "You can adapt ubuntu-setup.sh manually."
    exit 1
    ;;
esac
