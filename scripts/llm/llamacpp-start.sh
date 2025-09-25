#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# File: scripts/llm/llamacpp-start.sh
# PURPOSE: Start llama.cpp HTTP server bound to 127.0.0.1:11434
#
# Created by: Umicom Foundation (https://umicom.foundation/)
# Author: Sammy Hegab
# Date: 24-09-2025
# License: MIT
# -----------------------------------------------------------------------------
set -euo pipefail

: "${UENG_LLM_ENDPOINT:=http://127.0.0.1:11434}"
: "${MODEL:=}"

if [ -z "${MODEL}" ]; then
  echo "[error] MODEL environment variable not set (path to .gguf)."
  echo "        Example: MODEL=~/models/llama-3.2-1b-instruct.Q4_K_M.gguf $0"
  exit 1
fi

bin=".cache/llama.cpp/server"
if [ ! -x "$bin" ]; then
  echo "[error] server binary not found at $bin. Run scripts/llm/llamacpp-build.sh first."
  exit 1
fi

host="127.0.0.1"
port="11434"

echo "[info] Starting llama.cpp server on $host:$port with model: $MODEL"
"$bin" -m "$MODEL" -c 4096 --host "$host" --port "$port" >/dev/null 2>&1 &

for i in $(seq 1 30); do
  curl -fsS "$UENG_LLM_ENDPOINT" -m 1 >/dev/null 2>&1 && break || true
  sleep 0.5
done

echo "[ok] Server ready at $UENG_LLM_ENDPOINT"
