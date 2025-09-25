#!/usr/bin/env bash
# ------------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine) - Unix/macOS runner helper (Bash)
#
# PURPOSE:
#   - Robustly locates a built 'uaengine' in common build folders.
#   - If not found and UENG_RUN_AUTOBUILD=1 is set, it will configure+build
#     using Ninja (if available) or the default CMake generator.
#   - Forwards all remaining arguments to the binary.
#
# USAGE:
#   ./scripts/run-uaengine.sh -- --version
#   UENG_RUN_AUTOBUILD=1 ./scripts/run-uaengine.sh -- llm-selftest gpt-4o-mini
#
# ENV:
#   UENG_RUN_VERBOSE=1    -> prints extra diagnostics
#   UENG_RUN_AUTOBUILD=1  -> tries to build if the binary is missing
#   CMAKE_BUILD_TYPE      -> defaults to Release for Ninja configure
#   CC / CXX              -> honored during autobuild if set
#   UAENG_CMAKE_FLAGS     -> extra cmake flags (space-separated), e.g.:
#                            UAENG_CMAKE_FLAGS="-DUAENG_ENABLE_OPENAI=ON -DUAENG_ENABLE_OLLAMA=ON"
#
# Created by: Umicom Foundation (https://umicom.foundation/)
# Author: Sammy Hegab
# Date: 25-09-2025
# License: MIT
# ------------------------------------------------------------------------------

# We avoid `set -e` to print friendlier errors. We still treat unset vars carefully.
set -u

# ------------------------------ Helpers ---------------------------------------
log() { if [ "${UENG_RUN_VERBOSE:-0}" = "1" ]; then echo "[run] $*"; fi; }

# Resolve script and repo directories portably (works for symlinks too).
# NOTE: We keep this simple and reliable across Linux & macOS.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

# Candidate locations where uaengine may live after different builds.
CANDIDATES=(
  "${REPO_ROOT}/build/uaengine"
  "${REPO_ROOT}/build/Debug/uaengine"
  "${REPO_ROOT}/build/Release/uaengine"
  "${REPO_ROOT}/build-ninja/uaengine"
)

find_uaengine() {
  for p in "${CANDIDATES[@]}"; do
    if [ -x "$p" ]; then
      echo "$p"
      return 0
    fi
  done
  return 1
}

# ------------------------------- Run flow -------------------------------------
EXE="$(find_uaengine || true)"
if [ -n "${EXE:-}" ]; then
  log "Found uaengine: $EXE"
  exec "$EXE" "$@"
fi

# Optional autobuild if requested
if [ "${UENG_RUN_AUTOBUILD:-0}" = "1" ]; then
  log "uaengine not found; attempting autobuild…"

  if command -v ninja >/dev/null 2>&1; then
    log "Using Ninja generator"
    BUILD_DIR="${REPO_ROOT}/build-ninja"
    # Base configure args
    CMAKE_ARGS=(-S "${REPO_ROOT}" -B "${BUILD_DIR}" -G Ninja -D CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE:-Release}")
  else
    log "Ninja not found; using default CMake generator"
    BUILD_DIR="${REPO_ROOT}/build"
    CMAKE_ARGS=(-S "${REPO_ROOT}" -B "${BUILD_DIR}")
  fi

  # Honor CC/CXX if provided by the environment
  if [ -n "${CC:-}"  ]; then CMAKE_ARGS+=( -D CMAKE_C_COMPILER="${CC}" );  fi
  if [ -n "${CXX:-}" ]; then CMAKE_ARGS+=( -D CMAKE_CXX_COMPILER="${CXX}" ); fi

  # Pass-through extra flags, e.g., provider feature toggles
  if [ -n "${UAENG_CMAKE_FLAGS:-}" ]; then
    # shellcheck disable=SC2206 # we intentionally split on spaces here
    EXTRA_FLAGS=( ${UAENG_CMAKE_FLAGS} )
    CMAKE_ARGS+=( "${EXTRA_FLAGS[@]}" )
  fi

  log "cmake ${CMAKE_ARGS[*]}"
  if ! cmake "${CMAKE_ARGS[@]}"; then
    echo "[run] ERROR: CMake configure failed." >&2
    exit 1
  fi

  if ! cmake --build "${BUILD_DIR}" -j; then
    echo "[run] ERROR: CMake build failed." >&2
    exit 1
  fi

  EXE="$(find_uaengine || true)"
  if [ -n "${EXE:-}" ]; then
    log "Autobuild produced: $EXE"
    exec "$EXE" "$@"
  fi
fi

# Still not found → friendly guidance.
echo "[run] ERROR: could not find 'uaengine' in expected locations." >&2
echo "Tried:" >&2
for p in "${CANDIDATES[@]}"; do echo "  $p" >&2; done
echo >&2
echo "Hints:" >&2
echo "  1) Default generator:" >&2
echo "       cmake -S . -B build" >&2
echo "       cmake --build build -j" >&2
echo "  2) Ninja (recommended for fast iterating):" >&2
echo "       cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release" >&2
echo "       cmake --build build-ninja -j" >&2
exit 1
