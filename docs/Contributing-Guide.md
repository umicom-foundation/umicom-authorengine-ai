# Contributing Guide

Thanks for considering a contribution to **uaengine**! This guide explains how to propose changes, code style, and how we run the project.

---

## Ways to contribute

- **Bug reports** (preferred via GitHub Issues)
- **Small fixes** (typos, docs, tiny refactors)
- **Features** (open an issue first to discuss)
- **CI/Tooling** (smoke tests, sanitizers, linters)

---

## Development setup

### Build (Windows, Ninja + Clang)
```powershell
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
.\build\uaengine.exe --version
```

### Build (Windows, Visual Studio 2022)
```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release -j
.\build-vs\Release\uaengine.exe --version
```

### Build (Linux/macOS)
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/uaengine --version
```

Use the PowerShell helpers under `tools/` for convenience on Windows.

---

## Coding style

- C17, keep code portable (Windows/Linux/macOS).
- Run **clang-format** if available (we ship a `.clang-format`):
  - Windows (PowerShell loop if needed)
  - Linux/macOS: `git ls-files '*.c' '*.h' | xargs clang-format -i`
- Prefer small, focused functions; keep `#ifdef` islands minimal and localized.
- Add clear, short log messages for errors; return non‑zero on failure.

---

## Commit/branch workflow

- Fork → feature branch → PR
- Keep commits small and descriptive.
- Examples:
  - `fs: normalize extension checks on Windows`
  - `cli: add global/per-command help`
  - `ci: add smoke test workflow`
- Reference issues like: `Fixes #123` when appropriate.

---

## Tests & CI

- CI builds on Windows, Linux, macOS.
- Smoke tests verify `--version`, `--help`, and a minimal init→ingest→build→export flow.
- Optional jobs:
  - **Sanitizers** (ASan/UBSan on Linux)
  - **clang-tidy** (static analysis)
- Please make sure your PR is green on CI.

---

## Pull Request checklist

- [ ] Builds on all three OSes
- [ ] `clang-format` applied (or existing style obeyed)
- [ ] Minimal logs added for new errors
- [ ] README/docs updated if behavior is user‑visible

---

## Reporting security issues

Please do **not** open a public issue for sensitive problems.  
Email security contacts listed in `SECURITY.md` (or open a private advisory via GitHub).

---

## License

By contributing, you agree that your changes are licensed under the **MIT** License (see `LICENSE`).
