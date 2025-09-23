# Changelog

All notable changes to **uaengine** will be documented in this file.

## [v0.1.4] - 2025-09-23
### Added
- Global and per-command help: `uaengine --help`, `uaengine help <cmd>`, and `<cmd> --help` route to clear usage.
- Code scanning with **CodeQL** workflow (`.github/workflows/codeql.yml`).
- `.clang-format` for consistent C style (LLVM base, 2-space, Allman).

### Changed
- README updated with **Quality & Security** section and tools documentation.
- CI uploads both raw binaries and OS-named zipped artefacts (optional workflow step).

### Fixed
- Windows build scripts: robust quoting/argument handling; `pack.ps1` now locates VS multi-config output (`build-vs/Release/uaengine.exe`).
- Minor portability/cleanup in file-system helpers (case-insensitive extension checks).

## [v0.1.3] - 2025-09-23
### Fixed
- Build reliability on Windows (Ninja + Clang) and CI matrix.

*For earlier history, see git log.*
