## Developer Setup (multi-OS)

- Windows:
  ```powershell
  Set-ExecutionPolicy Bypass -Scope Process -Force
  .\scripts\setup\windows\install.ps1
  wsl --install -d Ubuntu
  bash .\scripts\setup\windows\wsl-ubuntu-setup.sh
  ```
- Linux:
  ```bash
  sudo bash scripts/setup/linux/bootstrap.sh
  ```
- macOS:
  ```bash
  bash scripts/setup/macos/brew-setup.sh
  ```

Then build:
```bash
cmake --preset default
cmake --build build -j
```

Optional:
```bash
pip install pre-commit
pre-commit install
```
