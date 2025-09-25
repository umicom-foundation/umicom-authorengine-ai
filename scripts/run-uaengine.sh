#!/usr/bin/env bash
#------------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# File: scripts/run-uaengine.sh
# PURPOSE: POSIX shell launcher to find and run the built 'uaengine' binary
#          from common build directories on Linux/macOS.
#
# Created by: Umicom Foundation (https://umicom.foundation/)
# Author: Sammy Hegab
# Date: 25-09-2025
# License: MIT
#------------------------------------------------------------------------------

set -euo pipefail

# Resolve repo root from this script location: <repo>/scripts/run-uaengine.sh
SCRIPT_DIR="$(cd -- "$(dirname "${BASH_SOURCE[0]}")" >/dev/null 2>&1 && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." >/dev/null 2>&1 && pwd)"

run_verbose() { [[ -n "${UENG_RUN_VERBOSE:-}" ]] && echo "[run] $*" >&2 || true; }

# Candidate paths to probe (first hit wins). We include both with/without .exe
# to play nice when scripts are used on Windows via Git Bash, etc.
CANDIDATES=(
  "build/uaengine"
  "build/uaengine.exe"
  "build/Release/uaengine"
  "build/Release/uaengine.exe"
  "build/Debug/uaengine"
  "build/Debug/uaengine.exe"
  "build-ninja/uaengine"
  "build-ninja/uaengine.exe"
)

BIN=""
for rel in "${CANDIDATES[@]}"; do
  abs="${REPO_ROOT}/${rel}"
  run_verbose "checking $abs"
  if [[ -f "$abs" ]]; then
    BIN="$abs"
    break
  fi
done

if [[ -z "$BIN" ]]; then
  echo "[run] ERROR: could not find 'uaengine' in expected build folders." >&2
  echo "Tried:" >&2
  for rel in "${CANDIDATES[@]}"; do echo "  - $REPO_ROOT/$rel" >&2; done
  cat >&2 <<'EOF'

Hints:
  1) Build with MSBuild (Visual Studio generator on Windows):
       cmake -S . -B build
       cmake --build build -j
     The binary will be in 'build/Debug/uaengine.exe' (or Release).

  2) Build with Ninja (Linux/macOS or Windows + Ninja):
       cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release
       cmake --build build-ninja -j
     The binary will be in 'build-ninja/uaengine' (or .exe on Windows).
EOF
  exit 1
fi

run_verbose "resolved to $BIN"

# Exec replaces this shell, preserving exit code and signals
exec "$BIN" "$@"
