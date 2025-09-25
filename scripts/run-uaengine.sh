#!/usr/bin/env bash
#-----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# File: scripts/run-uaengine.sh
# PURPOSE: Locate and run the built 'uaengine' binary on Unix-like systems.
#
# Created by: Umicom Foundation (https://umicom.foundation/)
# Author: Sammy Hegab
# Date: 24-09-2025
# License: MIT
#-----------------------------------------------------------------------------

set -euo pipefail

# Discover this script directory (robust across symlinks / sourcing)
SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
# Compute repo root (parent of scripts/)
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd -P)"

# If git is available, prefer the top-level root
if command -v git >/dev/null 2>&1; then
  if GROOT="$(git -C "$REPO_ROOT" rev-parse --show-toplevel 2>/dev/null)"; then
    REPO_ROOT="$GROOT"
  fi
fi

# Candidate paths to probe
candidates=(
  "$REPO_ROOT/build/uaengine"
  "$REPO_ROOT/build/uaengine.exe"       # in case built on Windows path
  "$REPO_ROOT/build/Debug/uaengine"
  "$REPO_ROOT/build/Debug/uaengine.exe"
  "$REPO_ROOT/build/Release/uaengine"
  "$REPO_ROOT/build/Release/uaengine.exe"
  "$REPO_ROOT/build-ninja/uaengine"
  "$REPO_ROOT/build-ninja/uaengine.exe"
  # cwd fallbacks
  "build/uaengine"
  "build/uaengine.exe"
  "build/Debug/uaengine"
  "build/Debug/uaengine.exe"
  "build/Release/uaengine"
  "build/Release/uaengine.exe"
  "build-ninja/uaengine"
  "build-ninja/uaengine.exe"
)

if [[ -n "${UENG_RUN_VERBOSE:-}" ]]; then
  echo "[run] Probing for 'uaengine' in:"
  for p in "${candidates[@]}"; do echo "  - $p"; done
fi

exe=""
for p in "${candidates[@]}"; do
  if [[ -f "$p" ]]; then
    exe="$p"
    break
  fi
done

if [[ -z "$exe" ]]; then
  echo "[run] ERROR: could not find 'uaengine' in expected build folders." >&2
  echo "Tried:" >&2
  for p in "${candidates[@]}"; do echo "  $p" >&2; done
  cat >&2 <<'EOF'

Hints:
  1) Build with MSBuild (Visual Studio generator on Windows):
       cmake -S . -B build
       cmake --build build -j
     The binary will be in 'build/Debug/uaengine(.exe)' (or Release).

  2) Build with Ninja (cross-platform):
       cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
       cmake --build build-ninja -j
     The binary will be in 'build-ninja/uaengine(.exe)'.
EOF
  exit 1
fi

if [[ -n "${UENG_RUN_VERBOSE:-}" ]]; then
  echo "[run] Using: $exe"
fi

# Support both: run-uaengine.sh -- --version   and   run-uaengine.sh --version
if [[ "${1:-}" == "--" ]]; then
  shift
fi

exec "$exe" "$@"
# End of script