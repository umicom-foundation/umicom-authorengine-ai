#!/usr/bin/env bash
set -euo pipefail
APP=uaengine
BUILD_DIR=build
OUT=dist
VER=$(${BUILD_DIR}/${APP} --version 2>/dev/null | tr ' ' '_' || echo "v0.0.0")

mkdir -p "$OUT"
tar -C "$BUILD_DIR" -czf "${OUT}/${APP}-${VER}-linux-x64.tar.gz" "$APP"
echo "Created ${OUT}/${APP}-${VER}-linux-x64.tar.gz"
