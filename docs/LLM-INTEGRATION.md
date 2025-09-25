# LLM Integration (Local + Cloud)

> **Additive only.** This document shows how to use the new tiny config layer without changing your existing flow. The rest of the code still works off environment variables; we simply provide defaults from `config/ueng.yaml` and a helper to export env if needed.

## Precedence

```
defaults  <  config/ueng.yaml  <  environment variables
```

You can keep your secrets (like `OPENAI_API_KEY`) in env vars and the rest in the YAML.

## Keys supported (flat)

```yaml
llm.provider: openai|ollama|llama
llm.model:    gpt-4o-mini|qwen2.5:3b|/path/model.gguf

openai.api_key:    ""      # prefer env OPENAI_API_KEY
openai.base_url:   ""      # optional

ollama.host: "http://127.0.0.1:11434"

llama.model_path: ""       # path to .gguf if embedding llama.cpp

serve.host: 127.0.0.1
serve.port: 8080

paths.workspace_dir: workspace
paths.site_root:     site
```

## Environment variables (override file)

- `UENG_LLM_PROVIDER`, `UENG_LLM_MODEL`
- `OPENAI_API_KEY`, `UENG_OPENAI_BASE_URL`
- `UENG_OLLAMA_HOST`
- `UENG_LLAMA_MODEL_PATH`
- `UENG_SERVE_HOST`, `UENG_SERVE_PORT`
- `UENG_WORKSPACE_DIR`, `UENG_SITE_ROOT`

## Wiring (optional) in `main.c`

Paste the tiny snippet from `snippets/main_integration.txt` near early argument parsing to load config and export env only if those env vars aren’t already set. This keeps legacy behavior intact and CI-friendly.

```c
#include "ueng/config.h"  /* new */

UengConfig cfg;
ueng_config_init_from("config/ueng.yaml", &cfg);
/* Export env only if not already set (overwrite=0). */
ueng_config_export_env(&cfg, 0);
```

Now `uaengine llm-selftest` will pick up provider/model from config unless you set env or pass CLI flags (CLI/env still win).

## Recipes

- **OpenAI Cloud**

  ```powershell
  $env:OPENAI_API_KEY="sk-..."     # or set persistently via Windows settings
  $env:UENG_LLM_PROVIDER="openai"
  .\build\uaengine.exe llm-selftest gpt-4o-mini
  ```

- **Ollama (local)**

  ```bash
  export UENG_LLM_PROVIDER=ollama
  ./build-ninja/uaengine llm-selftest qwen2.5:3b
  ```

- **llama.cpp (embedded)**

  ```bash
  export UENG_LLM_PROVIDER=llama
  ./build-ninja/uaengine llm-selftest /models/TinyLlama.gguf
  ```

## Security

We **do not** automatically export `OPENAI_API_KEY` from the YAML to your environment. Keep it in real env vars, a secret store, or CI secrets.
