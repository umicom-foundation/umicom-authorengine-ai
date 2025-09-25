#!/usr/bin/env bash
set -euo pipefail
: "${UENG_LLM_ENDPOINT:=http://127.0.0.1:11434}"
echo "[info] GET $UENG_LLM_ENDPOINT"
curl -fsS "$UENG_LLM_ENDPOINT" -m 5 >/dev/null || { echo "[error] not reachable"; exit 1; }
echo "[ok] reachable"
