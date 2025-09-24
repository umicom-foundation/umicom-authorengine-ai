#!/usr/bin/env bash
set -euo pipefail
if [ ! -x "./build/uaengine" ]; then
  echo "uaengine not found in ./build. Did you build it?"
  exit 0
fi
./build/uaengine --help || true
./build/uaengine --version || true
