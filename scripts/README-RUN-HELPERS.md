# Run helpers for `uaengine`

These tiny wrappers let you run **`uaengine`** without remembering where the
binary was emitted (MSBuild vs Ninja, Debug vs Release). They search common
output folders and forward your arguments unchanged.

> Created by: **Umicom Foundation** — Author: **Sammy Hegab** — Date: **25-09-2025** — License: **MIT**

---

## Files

- `scripts/run-uaengine.ps1` — Windows PowerShell launcher
- `scripts/run-uaengine.sh`  — Linux/macOS Bash launcher

Both are heavily commented so new contributors can read and learn.

---

## Usage

### Windows (PowerShell)

```powershell
# From repo root
powershell -ExecutionPolicy Bypass -File scripts\run-uaengine.ps1 -- --version
powershell -ExecutionPolicy Bypass -File scripts\run-uaengine.ps1 -- llm-selftest gpt-4o-mini
```

> The `--` is optional; it guarantees your args are passed on as-is.

### Linux / macOS

```bash
# First time only: make the file executable
chmod +x scripts/run-uaengine.sh

# Then run it
scripts/run-uaengine.sh --version
scripts/run-uaengine.sh llm-selftest gpt-4o-mini
```

---

## How resolution works

The scripts look for the first existing path (relative to the repo root):

```
build/uaengine(.exe)
build/Release/uaengine.exe
build/Debug/uaengine.exe
build-ninja/uaengine(.exe)
```

If nothing is found, a short help message with build commands is printed.

Set `UENG_RUN_VERBOSE=1` to see which paths are probed.

---

## Commit to repo

```powershell
git add scripts/run-uaengine.ps1 scripts/run-uaengine.sh scripts/README-RUN-HELPERS.md
git update-index --chmod=+x scripts/run-uaengine.sh   # so collaborators get the +x bit
git commit -m "tools(run): add cross-platform uaengine runner scripts with docs"
git push origin main
```
> To use OpenAI with llm-selftest:
> cmake -S . -B build -DUAENG_ENABLE_OPENAI=ON
> cmake --build build -j
> set OPENAI_API_KEY=sk-...   (Windows)   |   export OPENAI_API_KEY=...
> set UENG_LLM_PROVIDER=openai
> scripts\run-uaengine.ps1 -- llm-selftest gpt-4o-mini
