# LLM backends (OpenAI, Ollama, llama.cpp)

This project supports three provider backends behind a tiny C API:

- **OpenAI** (hosted): set `UENG_LLM_PROVIDER=openai` and `OPENAI_API_KEY`.
- **Ollama** (local server): set `UENG_LLM_PROVIDER=ollama` (default host `http://127.0.0.1:11434`).
- **llama.cpp** (embedded C library): set `UENG_LLM_PROVIDER=llama` and pass a `*.gguf` model path.

## Build flags

```bash
cmake -S . -B build -DUAENG_ENABLE_OPENAI=ON -DUAENG_ENABLE_OLLAMA=ON -DUAENG_ENABLE_LLAMA=ON
cmake --build build -j
```

Backends can be independently toggled. Sources always compile; when a backend
is disabled at CMake time, the corresponding functions will return a clear
runtime error message (so the CLI keeps working gracefully).

## Submodule (llama.cpp)

```bash
git submodule add https://github.com/ggml-org/llama.cpp third_party/llama.cpp
git submodule update --init --recursive
```

You **should commit** both the `.gitmodules` file and the *gitlink* entry for
`third_party/llama.cpp` so other developers can fetch the exact revision.

When `UAENG_ENABLE_LLAMA=ON` and the submodule is present, the embedded
library target `llama` is built and linked automatically.

## Running the CLI (helpers)

Use our cross‑platform helpers which auto‑locate the built binary:

- Windows (PowerShell):

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run-uaengine.ps1 -- --version
```

- macOS/Linux (bash):

```bash
scripts/run-uaengine.sh --version
```

You can pass any `uaengine` subcommand after the `--` separator on PowerShell
(or directly as arguments in bash).

## Environment variables

- `UENG_LLM_PROVIDER` = `openai` | `ollama` | `llama`
- `OPENAI_API_KEY`    = your key when using the OpenAI backend
- `OLLAMA_HOST`       = custom Ollama base URL (defaults to `http://127.0.0.1:11434`)
