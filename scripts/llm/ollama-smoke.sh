#!/usr/bin/env bash
set -euo pipefail
: "${UENG_LLM_ENDPOINT:=http://127.0.0.1:11434}"
echo "[info] GET $UENG_LLM_ENDPOINT/api/tags"
curl -fsS "$UENG_LLM_ENDPOINT/api/tags" -m 5 >/dev/null
echo "[ok] reachable"
echo "[info] POST /api/chat (tiny message)"
curl -fsS "$UENG_LLM_ENDPOINT/api/chat" -H 'Content-Type: application/json' -d '{"model":"llama3.2:1b","messages":[{"role":"user","content":"Say OK."}],"stream":false}' >/dev/null
echo "[ok] chat responded"
