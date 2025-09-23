# uaengine CLI Reference

This document complements `uaengine --help` by expanding examples and behavior.

## Global flags
- `--help` / `-h` – show top-level help
- `--version` / `-V` – show version (from `include/ueng/version.h`)

## Commands

### init
Initialize a new project in the current directory. Creates:
- `book.yaml`
- `dropzone/`
- `workspace/` and `workspace/chapters/`

Example:
```powershell
uaengine init
```

### ingest
Copy/normalize Markdown from `dropzone/` into `workspace/chapters/`.

Example:
```powershell
uaengine ingest
```

### build
Concatenate chapters into `workspace/book-draft.md` (simple order).

Example:
```powershell
uaengine build
```

### export
Produce HTML output in `outputs/<slug>/<YYYY-MM-DD>/html` and a minimal static site in `.../site`.

Example:
```powershell
uaengine export
```

### serve [host] [port]
Serve the latest site. Defaults to `127.0.0.1 8080`. Uses today’s dated folder unless overridden by `UENG_SITE_ROOT`.

Examples:
```powershell
uaengine serve
uaengine serve 0.0.0.0 8081
# Environment override:
$env:UENG_SITE_ROOT = "C:\path\to\outputs\my-new-book\2025-09-23\site"
uaengine serve
```

## Exit codes
- `0` success
- `>0` error (message printed to stderr / prefixed logs)

## Tips
- Prefer Ninja builds for speed on CI and locally.
- On Windows, use the PowerShell scripts under `tools/` for one-liner build/package/install.
