# Ollama: Local LLM backend (optional)

This add-on lets contributors run a **local LLM** with **Ollama** and point
AuthorEngine to it via `UENG_LLM_ENDPOINT`.

## Bundle contents
- scripts/llm/start-ollama.sh / .ps1 — start Ollama, pull small model, wait ready
- scripts/llm/ollama-smoke.sh / .ps1 — quick health check
- docker/docker-compose.ollama.yaml — reproducible Docker setup (localhost bind)
- .env.example-ollama — endpoint example

## Quick start
macOS/Linux:
```bash
curl -fsSL https://ollama.com/install.sh | sh
scripts/llm/ollama-smoke.sh
```

Windows:
```powershell
scripts\llm\ollama-smoke.ps1
```

Point tools:
```bash
export UENG_LLM_ENDPOINT=http://127.0.0.1:11434
```

Security: keep 11434 on localhost unless you know what you’re doing.
