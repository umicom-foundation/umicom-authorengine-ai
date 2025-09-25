# llama.cpp: optional local LLM backend (portable, CPU-first)

This add-on lets contributors use **llama.cpp** as a local backend for AuthorEngine.
It complements Ollama: Ollama is the simplest for contributors; **llama.cpp**
is ideal for **portable, vendorable, CPU-only** environments and future **embedding**.

---

## What is included

- `scripts/llm/llamacpp-build.sh` / `.ps1` — clone/update `ggml-org/llama.cpp` and build.
- `scripts/llm/llamacpp-start.sh` / `.ps1` — run the **HTTP server** on `127.0.0.1:11434`.
- `scripts/llm/llamacpp-smoke.sh` / `.ps1` — quick health check (same endpoint as Ollama).
- Common env hook: `UENG_LLM_ENDPOINT=http://127.0.0.1:11434`

> Models are **not** included. Place a **GGUF** file locally and point the start script to it.

---

## Quick start (POSIX)

```bash
# 1) Build (in repo root)
scripts/llm/llamacpp-build.sh

# 2) Run server with your GGUF model
MODEL=~/models/llama-3.2-1b-instruct.Q4_K_M.gguf scripts/llm/llamacpp-start.sh

# 3) Smoke test
scripts/llm/llamacpp-smoke.sh
```

## Quick start (Windows PowerShell)

```powershell
# 1) Build
scripts\llm\llamacpp-build.ps1

# 2) Run (point to your GGUF model path)
$env:MODEL="C:\Models\llama-3.2-1b-instruct.Q4_K_M.gguf"
scripts\llm\llamacpp-start.ps1

# 3) Smoke test
scripts\llm\llamacpp-smoke.ps1
```

---

## Embedding plan (bundle inside `uaengine`)

We can ship llama.cpp **inside** AuthorEngine via one of two modes:

### Mode A — **Spawned server** (fastest path, minimal code)
- Vendor llama.cpp under `third_party/llama.cpp/` or keep it as a git submodule.
- Extend CMake with an option (e.g., `UENG_WITH_LLAMA_SERVER=ON`), which builds the `server` target.
- On first run (or `ueng doctor`), if no model is configured, print guidance.
- When user opts in, `uaengine` spawns the bundled `server` binary on `127.0.0.1:11434` and talks HTTP.
- Pros: small surface area, same HTTP code as Ollama; easy cross-platform packaging.
- Cons: separate process.

### Mode B — **Linked library** (deeper integration)
- Build static lib from llama.cpp (`llama.o` and friends) and link it into `uaengine`.
- Use the C API (`llama.h`) to load the GGUF, run prompts, and stream tokens natively.
- Pros: single process; tighter control; easier sandboxing of prompts within CLI.
- Cons: more build complexity; larger binary; platform-specific acceleration toggles.

**Suggested first milestone: Mode A**. It gives immediate value without touching core code paths.

### CMake sketch for Mode A (server spawn)

```cmake
option(UENG_WITH_LLAMA_SERVER "Build bundled llama.cpp server" ON)

if(UENG_WITH_LLAMA_SERVER)
  include(ExternalProject)
  ExternalProject_Add(llamacpp
    GIT_REPOSITORY https://github.com/ggml-org/llama.cpp.git
    GIT_TAG master
    UPDATE_DISCONNECTED 1
    SOURCE_DIR ${CMAKE_BINARY_DIR}/_deps/llama.cpp-src
    BINARY_DIR ${CMAKE_BINARY_DIR}/_deps/llama.cpp-build
    CONFIGURE_COMMAND ""
    BUILD_COMMAND $(MAKE) -C <SOURCE_DIR> server
    INSTALL_COMMAND ""
  )
  add_custom_target(uaeng_llama_server ALL
    DEPENDS llamacpp
    COMMENT "Build llama.cpp server alongside uaengine")
endif()
```

### Runtime sketch (spawned server)

- Read env vars:
  - `UENG_LLM_PROVIDER=llamacpp`
  - `UENG_LLM_ENDPOINT=http://127.0.0.1:11434`
  - `UENG_LLM_MODEL=/path/to/model.gguf`
- If provider is `llamacpp` and the endpoint is not responding, spawn the bundled `server` with:
  `server -m "$UENG_LLM_MODEL" -c 4096 --port 11434 --host 127.0.0.1`
- Retry connection a few seconds, then continue.

### Licensing
- **llama.cpp** is MIT—compatible with our MIT license. Keep the upstream LICENSE in `third_party/llama.cpp/` when vendoring.

---

## CI usage

- For deterministic CI smoke tests, use **CPU** build (no GPU flags).
- Cache the `llama.cpp` build artifacts between runs (GitHub Actions cache key on commit hash + OS).
- Do **not** cache models; keep smoke tests model-free or use a tiny placeholder model kept in private cache.

---

## Environment variables

- `UENG_LLM_PROVIDER=ollama|llamacpp` (choose engine)
- `UENG_LLM_ENDPOINT=http://127.0.0.1:11434` (same for both backends)
- `MODEL=/path/to/model.gguf` (used by start scripts)
