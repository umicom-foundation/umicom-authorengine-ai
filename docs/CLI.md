# CLI Reference

This doc lists all commands and flags supported by **uaengine**.

```
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
```

## Global

- `-h, --help` – Show global usage.
- `-V, --version` – Print version string.

You can also run `uaengine help <command>`.

## Commands

### `init`
Create the initial project structure:
- `book.yaml` (with default slug `my-new-book`)
- `dropzone/`, `workspace/` (and `workspace/chapters/`)

**Usage**
```bash
uaengine init
```

### `ingest`
Copy/normalize Markdown from `dropzone/` into `workspace/chapters/`.

**Usage**
```bash
uaengine ingest
```

### `build`
Concatenate chapters into `workspace/book-draft.md`.

**Usage**
```bash
uaengine build
```

### `export`
Render simple HTML + a tiny static site under `outputs/<slug>/<YYYY-MM-DD>/{html,site}`.

**Usage**
```bash
uaengine export
```

### `serve`
Serve the latest site (or the path pointed by `UENG_SITE_ROOT`).

**Usage**
```bash
uaengine serve [host] [port]
# default host 127.0.0.1, default port 8080
```

Set an explicit site root:
```powershell
# Windows
$env:UENG_SITE_ROOT = "C:\path\to\outputs\my-new-book\2025-09-23\site"
uaengine serve
```
```bash
# Linux/macOS
export UENG_SITE_ROOT="/path/to/outputs/my-new-book/2025-09-23/site"
uaengine serve
```

### `publish`
Reserved for future integrations (no-op today).
