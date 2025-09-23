# Packaging

This guide explains how to package **uaengine** for Windows, macOS, and Linux,
and how to wire the artifacts into Scoop (Windows) and Homebrew (macOS/Linux).

---

## 0) Prerequisites

- CI must upload OS‑named zips on tags:
  - `uaengine-windows.zip`
  - `uaengine-macos.zip`
  - `uaengine-linux.zip`

If you used the upgraded `build.yml` these are produced automatically for `v*` tags.

---

## 1) Cut a release

1. Bump the version in `include/ueng/version.h` (e.g., `uaengine v0.1.4`).
2. Commit and push to `main`.
3. Tag and push:
   ```powershell
   git tag -a v0.1.4 -m "Release v0.1.4"
   git push origin v0.1.4
   ```
4. Wait for CI to finish and attach all three zip assets to the release.

---

## 2) Compute checksums

You can either hash local files or fetch from the release.

### Local (Windows PowerShell)
```powershell
# Example for Windows artifact you zipped locally
Get-FileHash .\dist\uengine-windows.zip -Algorithm SHA256
```

### From the GitHub release (helper script)
Use the helper we ship:
```powershell
powershell -ExecutionPolicy Bypass -File .\packaging\calc_hashes_from_release.ps1 -Version v0.1.4
```
This prints SHA256 for Windows/macOS/Linux zips.

---

## 3) Scoop (Windows)

**Option A: Keep the manifest in this repo (simple)**

- File: `packaging/uaengine.json`
- Replace `"hash": "<REPLACE_WITH_SHA256>"` with the Windows zip’s SHA256.

Install locally for testing:
```powershell
scoop install .\packaging\uaengine.json
uaengine --version
```

**Option B: Create a bucket (recommended for users)**

1. Create a new repo, e.g. `umicom-foundation/scoop-bucket`.
2. Place `uaengine.json` under `bucket/uaengine.json`.
3. Users can:
   ```powershell
   scoop bucket add umicom https://github.com/umicom-foundation/scoop-bucket
   scoop install uaengine
   ```

---

## 4) Homebrew (macOS/Linux)

**Tap repo (recommended):**
1. Create a repo `umicom-foundation/homebrew-tap`.
2. Path: `Formula/uaengine.rb`.
3. Fill in the two `sha256` values with the macOS and Linux zip checksums.

Test locally:
```bash
brew tap umicom-foundation/tap
brew install --build-from-source ./Formula/uaengine.rb   # or brew install uaengine (once published)
uaengine --version
```

**Notes**
- The formula expects the zip to contain a single binary named `uaengine`.
- If you change internal layout, adjust `bin.install` accordingly.

---

## 5) Release checklist

- [ ] Version header bumped
- [ ] Tag pushed (e.g., `v0.1.4`)
- [ ] CI artifacts present: windows/macos/linux zips
- [ ] Hashes updated in:
  - [ ] `packaging/uaengine.json` (Scoop)
  - [ ] `packaging/uaengine.rb` (Homebrew)
- [ ] Release Notes pasted (e.g., from `RELEASE_NOTES_vX.Y.Z.md`)
- [ ] Smoke test on each platform:
  ```bash
  uaengine --version
  uaengine --help
  ```

---

## 6) Troubleshooting

- **Missing zip assets on the release**  
  Ensure the tag matched the workflow’s trigger (`v*`) and that the packaging steps are enabled.
- **Scoop install fails with hash mismatch**  
  Recompute SHA256 and update `"hash"` in `uaengine.json`.
- **Homebrew `sha256` mismatch**  
  Update the `sha256` values in the formula to match the latest zips.
