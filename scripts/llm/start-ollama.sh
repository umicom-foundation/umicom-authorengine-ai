#!/usr/bin/env bash
# Umicom AuthorEngine AI (uaengine) — start-ollama.sh
set -euo pipefail
: "${UENG_LLM_ENDPOINT:=http://127.0.0.1:11434}"
echo "[info] Target endpoint: $UENG_LLM_ENDPOINT"
if command -v ollama >/dev/null 2>&1; then
  if curl -fsS "$UENG_LLM_ENDPOINT/api/tags" -m 2 >/dev/null 2>&1; then
    echo "[ok] Ollama running."
  else
    (ollama serve >/dev/null 2>&1 &) || true
    for i in $(seq 1 30); do
      curl -fsS "$UENG_LLM_ENDPOINT/api/tags" -m 1 >/dev/null 2>&1 && break
      sleep 0.5
    done
  fi
  MODEL="${1:-llama3.2:1b}"
  if ! ollama list | awk '{print $1}' | grep -qx "$MODEL"; then
    echo "[info] Pulling model: $MODEL"
    ollama pull "$MODEL"
  fi
  echo "[ok] Endpoint: $UENG_LLM_ENDPOINT"
else
  echo "[warn] 'ollama' not in PATH. Try Docker compose (docker/docker-compose.ollama.yaml)."
fi
