# Getting Started

Welcome! This guide walks you from zero → running **uaengine** on Windows, Linux, or macOS.

---

## Prerequisites

### Common
- CMake ≥ 3.20
- A C17-capable compiler (Clang / MSVC / GCC)

### Windows
- **Option A (fast)**: Ninja (`choco install ninja`) + LLVM/Clang
- **Option B**: Visual Studio 2022 (with C++ workload)

### Linux
```bash
sudo apt-get update && sudo apt-get install -y build-essential cmake ninja-build
```

### macOS
```bash
brew install cmake ninja
```

---

## Install (prebuilt binaries)

Grab the latest release assets:
- https://github.com/umicom-foundation/umicom-authorengine-ai/releases

After download:
- **Windows**: unzip `uaengine-windows.zip`, then run `uaengine.exe --version`.
- **macOS/Linux**: unzip and `chmod +x uaengine`, then `./uaengine --version`.

> Optional packaging (when you publish taps/buckets):
> - **Windows (Scoop)**:
>   ```powershell
>   scoop install https://raw.githubusercontent.com/umicom-foundation/umicom-authorengine-ai/main/packaging/uaengine.json
>   ```
> - **macOS/Linux (Homebrew)**:
>   ```bash
>   brew tap umicom-foundation/tap
>   brew install uaengine
>   ```

---

## Build from source

### Windows (Ninja + Clang)
```powershell
# From repo root
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
.\build\uaengine.exe --version
```

### Windows (Visual Studio 2022)
```powershell
cmake -S . -B build-vs -G "Visual Studio 17 2022" -A x64
cmake --build build-vs --config Release -j
.\build-vs\Release\uaengine.exe --version
```

### Linux
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/uaengine --version
```

### macOS
```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/uaengine --version
```

---

## One-liner (Windows PowerShell helpers)

From repo root:

```powershell
# Ninja, Release, pack + install to %USERPROFILE%\bin
powershell -ExecutionPolicy Bypass -File .\tools\build.ps1

# Visual Studio generator into .\build-vs
powershell -ExecutionPolicy Bypass -File .\tools\build.ps1 -Generator "Visual Studio 17 2022" -BuildDir .\build-vs

# Zip + install
powershell -ExecutionPolicy Bypass -File .\tools\pack.ps1 -Zip -InstallToUserBin
```

---

## First project in 60 seconds

```powershell
uaengine init

# Add some Markdown into dropzone/
Set-Content -NoNewline -Path .\dropzone\intro.md -Value "# Hello`n`nThis is a test."

uaengine ingest
uaengine build
uaengine export

# Preview the site (defaults to 127.0.0.1:8080)
uaengine serve
# or choose host/port:
uaengine serve 0.0.0.0 8080
```

### Serving a specific site folder
```powershell
# Windows (PowerShell)
$env:UENG_SITE_ROOT = "C:\path\to\outputs\my-new-book\2025-09-23\site"
uaengine serve
```
```bash
# Linux/macOS (bash)
export UENG_SITE_ROOT="/path/to/outputs/my-new-book/2025-09-23/site"
uaengine serve
```

---

## Troubleshooting

- **“Unknown command: --help”**  
  Update to v0.1.4+ (global/per-command help is supported).  
- **“generator used previously”** when switching CMake generators  
  Use a **different build directory** (e.g., `build-vs` vs `build`), or delete `CMakeCache.txt` and `CMakeFiles/`.
- **`serve` can’t find site root**  
  Run `uaengine export` today or set `UENG_SITE_ROOT` to a specific site path.
- **Windows PATH confusion**  
  If `uaengine --version` shows an older binary, check `Get-Command uaengine` and adjust PATH or reinstall via `tools\install.ps1`.

---

## Where next?

- Read the [CLI reference](CLI.md)  
- Explore internals in [Architecture](Architecture.md)  
- Contribute via the root [CONTRIBUTING.md](../CONTRIBUTING.md)
