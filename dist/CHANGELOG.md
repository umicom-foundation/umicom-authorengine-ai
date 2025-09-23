# Changelog

All notable changes to **Umicom AuthorEngine AI (uaengine)** will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned

- PDF export pipeline.
- Richer HTML theming and assets pipeline.
- CLI ergonomics: `serve --root <dir>`, improved error messages.
- Library-first refactor for integration with Umicom Studio (GTK4).

---

## [v0.1.2] - 2025-09-23

### Added

- **GitHub Actions CI** on Windows, Linux, and macOS; uploads build artefacts on every push/PR and attaches binaries to tagged releases.
- **README.md** overhaul with quick start, build instructions, usage, CI details, and release process.

### Changed

- **CMake**: set `_CRT_SECURE_NO_WARNINGS` on Windows via `add_compile_definitions()` to silence noisy UCRT deprecation warnings.

### Fixed

- **Windows build**: replaced non-portable `strcasecmp` usage and removed unused helper in `src/fs.c` so it compiles cleanly with Clang/MSVC toolchains.
- Minor comment cleanup and small portability improvements in file system handling.

---

## [v0.1.1] - 2025-09-22

### Added

- Initial public release of uaengine.
- Commands: `init`, `ingest`, `build`, `export`, `serve`, and `--version`.
- Minimal HTML site export with basic CSS theme.
- Cross-platform CMake project structure.

[Unreleased]: https://github.com/umicom-foundation/umicom-authorengine-ai/compare/v0.1.2...HEAD
[v0.1.2]: https://github.com/umicom-foundation/umicom-authorengine-ai/releases/tag/v0.1.2
[v0.1.1]: https://github.com/umicom-foundation/umicom-authorengine-ai/releases/tag/v0.1.1
