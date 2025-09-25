# Run helpers

These helpers let you run **uaengine** from anywhere in the repo.

## Windows (PowerShell)

```powershell
powershell -ExecutionPolicy Bypass -File scripts\run-uaengine.ps1 -- --version
$env:UENG_RUN_AUTOBUILD="1"; scripts\run-uaengine.ps1 -- llm-selftest gpt-4o-mini
```

## Linux / macOS (Bash)

```bash
bash scripts/run-uaengine.sh -- --version
UENG_RUN_AUTOBUILD=1 bash scripts/run-uaengine.sh -- llm-selftest gpt-4o-mini
```

## Environment variables

- `UENG_RUN_VERBOSE=1` – extra logs
- `UENG_RUN_AUTOBUILD=1` – configure+build if binary is missing
- `UAENG_CMAKE_FLAGS` – extra CMake flags (space-separated), e.g.  
  `-DUAENG_ENABLE_OPENAI=ON -DUAENG_ENABLE_OLLAMA=ON -DUAENG_ENABLE_LLAMA=ON`
- `CMAKE_BUILD_TYPE` – used with Ninja (default: `Release`)
- `CC`, `CXX` – compilers to use during autobuild

## Notes

The helpers probe common outputs:
- `build/uaengine(.exe)`
- `build/Debug/uaengine(.exe)`
- `build/Release/uaengine(.exe)`
- `build-ninja/uaengine(.exe)`

If not found and autobuild is enabled, they run CMake with Ninja when available (otherwise the default generator).

---
**Credits**: Umicom Foundation — AuthorEngine AI. MIT License.
