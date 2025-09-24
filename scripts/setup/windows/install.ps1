# Windows developer setup (PowerShell, run as Administrator)
$ErrorActionPreference = "Stop"

function Install-App($IdOrName) {
  try {
    Write-Host "Installing: $IdOrName" -ForegroundColor Green
    winget install --id $IdOrName --accept-package-agreements --accept-source-agreements -e -h `
      -o "C:\Program Files" 2>$null
  } catch {
    Write-Host " -> Try: winget search $IdOrName ; then winget install <exact id>" -ForegroundColor Yellow
  }
}

Write-Host "==> Core shells/tools" -ForegroundColor Cyan
Install-App "Microsoft.PowerShell"
Install-App "Microsoft.WindowsTerminal"
Install-App "Git.Git"
Install-App "GitHub.cli"

Write-Host "==> Build toolchain" -ForegroundColor Cyan
Install-App "Kitware.CMake"
Install-App "Ninja-build.Ninja"
Install-App "LLVM.LLVM"
Install-App "Microsoft.VisualStudio.2022.BuildTools"

Write-Host "==> Editors/IDEs" -ForegroundColor Cyan
Install-App "Microsoft.VisualStudioCode"
Install-App "Notepad++.Notepad++"

Write-Host "==> Runtimes" -ForegroundColor Cyan
Install-App "Python.Python.3.11"
Install-App "OpenJS.NodeJS.LTS"
Install-App "Microsoft.OpenJDK.21"
Install-App "Microsoft.DotNet.SDK.8"

Write-Host "==> Containers" -ForegroundColor Cyan
Install-App "Docker.DockerDesktop"

Write-Host "==> Databases/clients" -ForegroundColor Cyan
Install-App "SQLiteBrowser.SQLiteBrowser"
Install-App "DBeaver.DBeaver"

Write-Host "==> Media & utilities" -ForegroundColor Cyan
Install-App "7zip.7zip"
Install-App "voidtools.Everything"
Install-App "VideoLAN.VLC"
Install-App "OBSProject.OBSStudio"
Install-App "HandBrake.HandBrake"
Install-App "ShareX.ShareX"
Install-App "GIMP.GIMP"
Install-App "Inkscape.Inkscape"

Write-Host "==> MSYS2 (for GTK4 native)" -ForegroundColor Cyan
Install-App "MSYS2.MSYS2"

Write-Host "==> Enable WSL2 features (reboot after this step)" -ForegroundColor Cyan
dism.exe /online /enable-feature /featurename:Microsoft-Windows-Subsystem-Linux /all /norestart
dism.exe /online /enable-feature /featurename:VirtualMachinePlatform /all /norestart

Write-Host "==> Done. Reboot recommended if WSL/VM features changed." -ForegroundColor Green
