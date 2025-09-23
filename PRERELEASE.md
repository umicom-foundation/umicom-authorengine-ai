# v0.1.4-rc1 – Pre-release Notes
_Date: 2025-09-23_

> **Status**: Pre-release (candidate). Built by CI on Windows, Linux, and macOS. Use for testing; not recommended for production distribution yet.

## Summary
This pre-release focuses on developer experience and packaging:
- Convenience PowerShell tooling under `tools/` (`build.ps1`, `pack.ps1`, `install.ps1`, `uninstall.ps1`, `make.ps1`).
- Visual Studio builds in a separate `build-vs` folder.
- CI now uploads **raw binaries and zipped artefacts** per OS.
- Repository hygiene and docs: updated README, CHANGELOG, CONTRIBUTING, SECURITY, issue templates.
- Optional style guard via `.clang-format` (LLVM base).

## Highlights
- **One-liner build & install** on Windows:
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\tools\build.ps1
  # or with VS generator:
  powershell -ExecutionPolicy Bypass -File .\tools\build.ps1 -Generator "Visual Studio 17 2022" -BuildDir .\build-vs
  ```
- **Package & zip** in one go:
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\tools\pack.ps1 -Zip -InstallToUserBin
  ```
- **Global install** to `%USERPROFILE%\bin`:
  ```powershell
  powershell -ExecutionPolicy Bypass -File .\tools\install.ps1
  ```

## Changes since v0.1.3
### Added
- `tools/` scripts for build/pack/install/uninstall/make.
- GitHub Actions now upload **zipped artefacts** (`uaengine-<OS>.zip`).

### Changed
- README updated with tools usage and VS build directory (`build-vs`).

### Fixed
- Windows: robust path/quoting in scripts; packer detects VS multi-config output (`build-vs\Release\uaengine.exe`).

## Known Issues / To Do
- Top-level `uaengine --help` prints usage for subcommands only; consider adding a global help switch.
- PDF export pipeline not yet implemented.
- Theme/custom CSS pipeline minimal; more layouts planned.

## Upgrade Notes
No breaking changes to CLI. If you installed a previous binary to `%USERPROFILE%\bin`, you can overwrite it via:
```powershell
powershell -ExecutionPolicy Bypass -File .\tools\pack.ps1 -InstallToUserBin
```

## Verification
1. Run the binary:
   ```powershell
   uaengine --version
   ```
2. Build the example flow:
   ```powershell
   uaengine init
   Set-Content -NoNewline -Path .\dropzone\intro.md -Value "# Hello`n`nThis is a test."
   uaengine ingest
   uaengine build
   uaengine export
   uaengine serve
   ```

## Checksums
The CI job will attach `.zip` artefacts. Compute checksums locally (example on Windows PowerShell):
```powershell
Get-FileHash .\dist\uaengine-windows.zip -Algorithm SHA256
```

---

Thanks for testing! Please file issues with logs and your OS/compiler details.
