# Contributing to Umicom AuthorEngine AI (uaengine)

First off, thanks for taking the time to contribute!

## Getting started

### Prerequisites
- CMake ≥ 3.20
- A C17-capable compiler (Clang, MSVC, or GCC)
- **Windows**: Ninja (`choco install ninja`) or Visual Studio 2022
- **Linux**: `build-essential cmake ninja-build`
- **macOS**: `brew install cmake ninja`

### Build (Ninja)
```powershell
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
.uild\uaengine.exe --version   # Windows
./build/uaengine --version        # Linux/macOS
```

### Build (Visual Studio 2022)
Use a separate build folder to avoid generator clashes:
```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release -j
.uild-vs\Release\uaengine.exe --version
```

### Helper scripts (Windows PowerShell)
All helpers live in `tools/`:
- `build.ps1` – Configure & build with CMake, then pack & install to `%USERPROFILE%\bin`.
- `pack.ps1` – Copy built binary to `dist\bin` (optional `-Zip`, `-InstallToUserBin`).
- `install.ps1` / `uninstall.ps1` – Manage a global install in `%USERPROFILE%\bin`.
- `make.ps1` – Task runner: `build`, `clean`, `zip`, `vsbuild`, `release vX.Y.Z`.

## Style
- C code format: see `.clang-format` (LLVM base, 2-space indents, Allman braces).
- Keep functions small and focused; prefer static helpers in translation units.
- Avoid platform-specific code paths unless needed; prefer portable C.

## Tests
A basic smoke test runs in CI (`--version`). Please add minimal unit/behavior tests when changing user-visible behavior.

## Branch & PR
- Branch from `main`.
- Keep changes small; one logical change per PR.
- Update `README.md` (and `CHANGELOG.md` if needed) for user-visible changes.
- CI must pass on Windows, Linux, and macOS.

## Releases
- Bump `include/ueng/version.h` (string used by `--version`).
- Tag `vX.Y.Z`. CI will build and attach artifacts to the GitHub Release.
