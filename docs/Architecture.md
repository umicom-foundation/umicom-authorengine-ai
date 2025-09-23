# Architecture

This document explains how **uaengine** is structured and how data flows from your Markdown inputs to the generated HTML/site.

---

## High-level overview

```text
+-----------+      +-----------+      +-----------------------+      +----------------------------+
| dropzone/ | ---> | ingest    | ---> | workspace/chapters/*.md | --> | build -> book-draft.md    |
+-----------+      +-----------+      +-----------------------+      +----------------------------+
                                                                         |
                                                                         v
                                                                    +-----------+
                                                                    | export    |
                                                                    |  html/    |
                                                                    |  site/    |
                                                                    +-----------+
                                                                         |
                                                                         v
                                                                    +-----------+
                                                                    | serve     |
                                                                    | http      |
                                                                    +-----------+
```

- **init**: scaffolds `book.yaml`, `dropzone/`, `workspace/`, etc.
- **ingest**: copies/normalizes Markdown from `dropzone/` → `workspace/chapters/`.
- **build**: concatenates chapters into `workspace/book-draft.md`.
- **export**: writes HTML and a tiny site under `outputs/<slug>/<YYYY-MM-DD>/{html,site}`.
- **serve**: static HTTP server for the most recent (or explicitly-set) site.

---

## Source layout

```
include/ueng/
  common.h   # small utilities/shared structs
  fs.h       # filesystem helpers: scanning, path ops, extension checks
  serve.h    # minimal static server interface
  version.h  # version string reported by --version

src/
  common.c
  fs.c       # ingestion/build/export helpers
  serve.c    # simple HTTP server (host/port, content-type map)
  main.c     # CLI entry: arg parsing, help routing, command dispatch
```

### main.c
- Parses `argv` and routes:
  - Global help/version: `--help`, `-h`, `--version`, `-V`
  - `help <command>`
  - `<command> [args]`
- Calls into `cmd_init / cmd_ingest / cmd_build / cmd_export / cmd_serve / cmd_publish`.

### fs.c
- Directory creation, file iteration, extension matching (portable, case-insensitive on Windows).
- Ingest/build/export primitives; relies on UTF-8 where possible.

### serve.c
- Minimal HTTP server:
  - Accepts `host` and `port`.
  - Serves files from resolved `site root`.
  - Basic `Content-Type` detection by extension.
- Site root discovery:
  - If `UENG_SITE_ROOT` env var set → use it.
  - Else derive `outputs/<slug>/<YYYY-MM-DD>/site` (today’s date).

### common.c/h
- Small helpers (strings, error/print macros).
- Keeps OS-specific ifdefs contained.

---

## Data model

- **book.yaml**: carries at least the project **slug**. Defaults to `my-new-book`.
- **chapters**: a set of normalized Markdown files under `workspace/chapters/`.
- **book-draft.md**: concatenation result, used by the exporter.

> Future work may add richer metadata (author, title pages, TOC order).

---

## Error handling

- Functions return non-zero on failure; CLI surfaces short, actionable messages.
- Use of `_CRT_SECURE_NO_WARNINGS` on Windows is configured via CMake to limit noisy warnings.
- `serve` prints hints if no site is found (e.g., “Run `uaengine export`” or set `UENG_SITE_ROOT`).

---

## Cross-platform concerns

- **Windows**:
  - Case-insensitive extension checks and path handling.
  - Prefer `_dupenv_s` (or guard `getenv` via build flag) as needed by toolchain.
- **Linux/macOS**:
  - Standard POSIX directory walking and file I/O (via portable wrappers where needed).

The code aims to keep platform `#ifdef`s localized and small.

---

## Extending the CLI

Add a new command `foo`:

1. **Declare** in `include/ueng/common.h` (prototype) or a dedicated header.
2. **Implement** `cmd_foo` in `src/foo.c` (new file) or an existing TU if tiny.
3. **Dispatch** in `main.c`:
   ```c
   if(strcmp(argv[1],"foo")==0){ return cmd_foo(argc-1, argv+1); }
   ```
4. **Help text**: add `usage_foo()` and wire it in `usage_cmd()` so `uaengine help foo` works.
5. **CMakeLists.txt**: add your `foo.c` to the source list.

---

## Build & test in CI

- Matrix builds (Win/Linux/macOS) with Ninja; optional VS on Windows.
- Smoke tests: `--version`, `--help`, and a tiny init→ingest→build→export flow.
- Sanitizers (Address/Undefined) on Linux can be enabled to catch UB and heap issues.
- Static analysis via clang-tidy on PRs.

---

## Future directions

- PDF export pipeline.
- Richer themes/layouts (CSS, templates).
- Chapter ordering rules (front-matter or `book.yaml` sections).
- Plug-in hooks for custom preprocessors.

---

For code-level details, start with:
- `src/main.c` for CLI flow.
- `src/fs.c` for content pipeline.
- `src/serve.c` for the web server.
