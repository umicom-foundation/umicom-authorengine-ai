# -----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# PURPOSE: Project helper script.
#
# Created by: Umicom Foundation (https://umicom.foundation/)
# Developer: Sammy Hegab
# Date: 24-09-2025
# License: MIT
# -----------------------------------------------------------------------------
# NOTE:
# Every line is commented so even a beginner can follow.
# -----------------------------------------------------------------------------

param([switch]$Am21)

# Figure out where we are on disk
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot  = Split-Path -Parent $ScriptDir

# Decide whether to run AM21 bundle installer or generic Windows installer
$useAm21 = $Am21 -or ($env:UENG_AM21 -eq "1")

if ($useAm21) {
  Write-Host "AM21 mode: running am21_bundle installer..." -ForegroundColor Cyan
  $am21 = Join-Path $RepoRoot "scripts\setup\am21_bundle\am21-install.ps1"
  & $am21
} else {
  Write-Host "Generic Windows setup..." -ForegroundColor Cyan
  $win = Join-Path $RepoRoot "scripts\setup\windows\install.ps1"
  & $win
}

Write-Host "Next steps (optional):" -ForegroundColor Yellow
Write-Host "  wsl --install -d Ubuntu" -ForegroundColor Yellow
Write-Host "  bash .\scripts\setup\windows\wsl-ubuntu-setup.sh" -ForegroundColor Yellow
