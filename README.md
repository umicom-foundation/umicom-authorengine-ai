# Umicom AuthorEngine AI (uaengine)

[![build](https://github.com/umicom-foundation/umicom-authorengine-ai/actions/workflows/build.yml/badge.svg)](https://github.com/umicom-foundation/umicom-authorengine-ai/actions/workflows/build.yml)

A tiny, portable command-line engine to help you turn loose Markdown into a book draft, export basic HTML, and serve a simple site locally. Built in C (C17) with zero heavy dependencies so it compiles fast on Windows, Linux, and macOS.

---

## Features (current)

- **Project scaffolding**: `init` creates a sensible workspace.
- **Ingestion**: `ingest` pulls Markdown from `dropzone/` into `workspace/chapters/`.
- **Draft build**: `build` concatenates chapters → `workspace/book-draft.md`.
- **HTML export**: `export` emits HTML + a simple `site/` with CSS per build.
- **Local preview**: `serve [host] [port]` serves the latest site (defaults to `127.0.0.1 8080`).
- **Cross-platform**: single C codebase with CMake; CI builds on Win/Linux/macOS.
- **MIT licensed**.

Planned: richer HTML theming, PDF export pipeline, and IDE/GUI integration (Umicom Studio).

---

## Quick start

### 1) Download a binary (recommended)
Head to **Releases** and grab the asset for your OS: https://github.com/umicom-foundation/umicom-authorengine-ai/releases

### 2) Or build from source

#### Windows (LLVM/Clang + Ninja)
```powershell
# From the repo root
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
.\build\uaengine.exe --version
```

If you prefer Visual Studio:
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j
.\build\Release\uaengine.exe --version
```

#### Linux (GCC/Clang + Ninja or Make)
```bash
# With Ninja
sudo apt-get update && sudo apt-get install -y build-essential cmake ninja-build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/uaengine --version
```

#### macOS (AppleClang)
```bash
# Ensure CMake + Ninja exist: brew install cmake ninja
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/uaengine --version
```

> **Note (Windows):** The project sets `_CRT_SECURE_NO_WARNINGS` via CMake to avoid noisy UCRT deprecation warnings. No functional change.

---

## Usage

```text
Umicom AuthorEngine AI (uaengine) - Manage your book projects with AI assistance.

Usage: uaengine <command> [options]

Commands:
  init                 Initialize a new book project structure.
  ingest               Ingest and organize content from the dropzone.
  build                Build the book draft and prepare outputs.
  export               Export the book to HTML and PDF formats.
  serve [host] [port]  Serve outputs/<slug>/<date>/site over HTTP (default 127.0.0.1 8080).
  publish              Publish the book to a remote server (not implemented).
  --version            Show version information.

Run 'uaengine <command> --help' for command-specific options.
```

### Typical flow
```powershell
# 1) Create scaffolding (book.yaml, workspace/, dropzone/, etc.)
uaengine init

# 2) Put your Markdown files into .\dropzone\
#    (example: create a quick one if you like)
#    Windows:
Set-Content -NoNewline -Path .\dropzone\intro.md -Value "# Hello`n`nThis is a test."

# 3) Ingest -> copies Markdown into workspace/chapters
uaengine ingest

# 4) Build -> writes workspace/book-draft.md
uaengine build

# 5) Export -> emits outputs/<slug>/<YYYY-MM-DD>/{html,site}
uaengine export

# 6) Serve -> hosts the site at http://127.0.0.1:8080 by default
uaengine serve
# or specify host/port:
uaengine serve 0.0.0.0 8080
```

### Serving a specific site folder
By default, `serve` finds today’s site at `outputs/<slug>/<YYYY-MM-DD>/site`.  
Override explicitly via environment variable:

- **Windows (PowerShell)**
  ```powershell
  $env:UENG_SITE_ROOT = "C:\path\to\outputs\my-new-book\2025-09-23\site"
  uaengine serve
  ```

- **Linux/macOS (bash)**
  ```bash
  export UENG_SITE_ROOT="/path/to/outputs/my-new-book/2025-09-23/site"
  uaengine serve
  ```

---

## Folder structure

```
.
├─ CMakeLists.txt
├─ include/
│  └─ ueng/
│     ├─ common.h
│     ├─ fs.h
│     ├─ serve.h
│     └─ version.h
├─ src/
│  ├─ common.c
│  ├─ fs.c
│  ├─ main.c
│  └─ serve.c
├─ dropzone/                 # put raw Markdown here (your input)
├─ workspace/
│  ├─ chapters/              # normalised chapters after 'ingest'
│  └─ book-draft.md          # created by 'build'
├─ outputs/
│  └─ <slug>/<YYYY-MM-DD>/
│       ├─ html/             # HTML export
│       └─ site/             # minimal site (index.html + cover.svg + css)
├─ .github/workflows/build.yml
└─ README.md
```

> `<slug>` is read from `book.yaml` (created by `init`). Default is `my-new-book`.

---

## Continuous Integration (CI)

This repository builds on every push/PR and on tags **`v*`** across:
- `windows-latest`, `ubuntu-latest`, `macos-latest`.

Artefacts are uploaded per job. On tag pushes, a GitHub Release is created and artefacts are attached.

Workflow file: `.github/workflows/build.yml`.

---

## Versioning & Releases

- The CLI reports its version via `--version` (string lives in `include/ueng/version.h`).
- To cut a release:
  ```powershell
  # Update include/ueng/version.h to the new version string first
  git add include/ueng/version.h
  git commit -m "version: bump to vX.Y.Z"
  git push origin main

  # Tag and push
  git tag -a vX.Y.Z -m "Release vX.Y.Z"
  git push origin vX.Y.Z
  ```
- CI will attach built binaries to the GitHub Release for that tag.

---

## Contributing

Pull requests are welcome. Please:
1. Keep changes small and focused.
2. Ensure it builds on Windows/Linux/macOS.
3. Run `clang-format` (if available) or follow the existing style.
4. Add a brief note to this README if you alter user-visible behaviour.

---

## Licence

MIT © Umicom Foundation
