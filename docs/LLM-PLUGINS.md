# LLM Backends (OpenAI, Ollama, llama.cpp)

`include/ueng/llm.h` defines a tiny, stable API used by the CLI. Implementations live in `src/llm_*.c`.

## Runtime selection

Set the provider with `UENG_LLM_PROVIDER`:

- `openai`  — uses environment variable `OPENAI_API_KEY` and model id (e.g., `gpt-4o-mini`)
- `ollama`  — requires a running Ollama daemon on `localhost:11434`
- `llama`   — links a compiled‑in llama.cpp backend and takes a `.gguf` model path

Example:

```powershell
$env:UENG_LLM_PROVIDER="openai"
.uild\uaengine.exe llm-selftest gpt-4o-mini
```

## Error handling

- `ueng_llm_open` returns `NULL` and fills `err` if the provider is unavailable or misconfigured.
- `ueng_llm_prompt` returns non‑zero with a short message in `out` on failure.

