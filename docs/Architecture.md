# Umicom AuthorEngine AI — Architecture & Layout

This document explains how the codebase is organized today and how each module fits together.

## High-level flow

```
uaengine (CLI)
 ├─ init/ingest/build/export/render/doctor  (src/main.c dispatcher)
 ├─ common utilities                         (include/ueng/common.h, src/common.c)
 ├─ filesystem helpers                       (include/ueng/fs.h,     src/fs.c)
 ├─ tiny HTTP server                         (include/ueng/serve.h,  src/serve.c)
 └─ LLM adapter layer                        (include/ueng/llm.h,    src/llm_*.c)
```

- **main.c** is a thin command dispatcher. Each command is implemented in a small function.
- **common.c/h** hosts small cross‑platform helpers (string list, time, proc, path).
- **fs.c/h** contains higher‑level helpers for workspace & site outputs.
- **serve.c/h** is a tiny static file server for local preview.
- **llm.h + llm_*.c** implement a minimal provider‑neutral interface for OpenAI, Ollama, and llama.cpp.

## Design principles

- **Loosely coupled**: headers expose small, stable APIs; implementation files can evolve independently.
- **Portable C17**: avoid non‑standard extensions. Minimal `#ifdef` blocks are centralized in headers.
- **Fail fast, explain clearly**: all functions return 0 on success (or non‑NULL), and provide friendly errors.
- **Readable first**: code is heavily commented; we prefer clarity over cleverness.

## Adding a new LLM provider

1. Create `src/llm_newprovider.c`.
2. Implement: `ueng_llm_open`, `ueng_llm_prompt`, `ueng_llm_close`.
3. Toggle it with `-DUAENG_ENABLE_NEWPROVIDER=ON` in CMake and add the source to the target.
4. Keep the public header `include/ueng/llm.h` vendor‑neutral.

