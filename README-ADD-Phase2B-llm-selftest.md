# Phase 2B: Add `llm-selftest` command (non-destructive patch)

These scripts **insert** a tiny command handler into `src/main.c` to test the
embedded llama.cpp wrapper. They do **not** delete or reflow any existing code;
they only add lines next to well-known anchors.

## What the command does

- `uaengine llm-selftest [model.gguf]`:
  - Loads the model (or uses `UENG_LLM_MODEL` env var if no path is given).
  - Runs a small prompt: "Say hello from AuthorEngine."
  - Prints the generated text to stdout.
  - Exits 0 on success, non-zero on failure.

## Usage

### Windows PowerShell
```powershell
powershell -ExecutionPolicy Bypass -File scripts\fix\add-llm-selftest.ps1
```

### macOS/Linux (bash)
```bash
bash scripts/fix/add-llm-selftest.sh
```

> Re-run safely: the scripts detect if the code is already applied.

## Build & try
```bash
cmake -S . -B build -DUENG_WITH_LLAMA_EMBED=ON
cmake --build build -j
./build/uaengine llm-selftest /path/to/model.gguf
```

If embedding is OFF or llama.cpp isn’t vendored, the runtime prints a friendly
stub message and returns non-zero (build still succeeds).