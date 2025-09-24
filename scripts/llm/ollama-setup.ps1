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

$ErrorActionPreference = "Stop"

# Download folder and installer target path
$downloads = Join-Path $env:USERPROFILE "Downloads"
$installer = Join-Path $downloads "OllamaSetup.exe"

Write-Host "Downloading Ollama to $installer..." -ForegroundColor Cyan
try {
  Invoke-WebRequest -Uri "https://ollama.com/download/OllamaSetup.exe" -OutFile $installer -UseBasicParsing
  Write-Host "Downloaded: $installer" -ForegroundColor Green
} catch {
  Write-Host "Could not download automatically. Visit https://ollama.com to install manually." -ForegroundColor Yellow
  exit 0
}

# Launch the installer GUI
Start-Process -FilePath $installer

Write-Host "After installation, open a fresh PowerShell and run:" -ForegroundColor Yellow
Write-Host "  ollama serve" -ForegroundColor Yellow
Write-Host "  ollama pull mistral" -ForegroundColor Yellow
