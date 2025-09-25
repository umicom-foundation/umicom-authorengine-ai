# Phase 2B: Add `llm-selftest` command (non-destructive patch) — v2

This version fixes PowerShell quoting issues. It safely inserts a new command
into `src/main.c` without removing or reformatting your existing code/comments.

## Usage

### Windows PowerShell
```powershell
powershell -ExecutionPolicy Bypass -File scripts\fix\add-llm-selftest.ps1
```

### macOS/Linux (bash)
```bash
bash scripts/fix/add-llm-selftest.sh
```

## Build & run on Windows
Your MSBuild generator places the binary at `build\Debug\uaengine.exe` by default:
```powershell
cmake -S . -B build
cmake --build build -j
.\build\Debug\uaengine.exe llm-selftest C:\path\to\model.gguf
```
