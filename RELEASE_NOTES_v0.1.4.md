# uaengine v0.1.4 — Release Notes
_Date: 2025-09-23_

This release focuses on developer experience, packaging, and a more helpful CLI.

## Highlights
- **Help everywhere:** `uaengine --help`, `uaengine help <command>`, and `<command> --help`.
- **Better CI artifacts:** raw binaries **and** OS-specific zip files (when using the upgraded workflow).
- **Quality & security:** CodeQL scanning and a `.clang-format` for consistent code style.
- **Smoother Windows builds:** scripts handle quoting correctly; Visual Studio builds use `build-vs/` and are automatically packed.

## Changes
### Added
- CodeQL workflow at `.github/workflows/codeql.yml`.
- `.clang-format` (LLVM base, 2-space, Allman).
- Global + per-command help routing in the CLI.

### Changed
- README includes “Quality & Security” and expanded **tools/** docs.

### Fixed
- `pack.ps1` resolves executables from Ninja and VS multi-config layouts.

## Download
See the assets attached to this GitHub Release for Windows, macOS, and Linux.

## Verify
```powershell
uaengine --version
uaengine --help
uaengine help serve
```

## Thanks
Thanks to everyone testing the RC and reporting issues. Keep feedback coming!
