# pack.ps1 — copy the latest build to dist\bin and (optionally) zip a release asset
param(
  [string]$BuildDir = ".\build",
  [switch]$Zip,                   # create dist\uengine-windows.zip
  [switch]$InstallToUserBin       # also copy to %USERPROFILE%\bin for global use
)

$ErrorActionPreference = "Stop"

# Determine platform binary name
$ExeName = "uaengine.exe"

# Validate build output
$SrcExe = Join-Path $BuildDir $ExeName
if (-not (Test-Path $SrcExe)) {
  Write-Error "Not found: $SrcExe. Build first: cmake -S . -B build -G Ninja; cmake --build build -j"
}

# Prepare dist\bin
$DistRoot = Join-Path $PSScriptRoot 'dist'
$DistBin  = Join-Path $DistRoot 'bin'
New-Item -ItemType Directory -Force -Path $DistBin | Out-Null

# Copy exe
$DstExe = Join-Path $DistBin $ExeName
Copy-Item -Force $SrcExe $DstExe
Write-Host "Packed: $DstExe"

# Include helpful docs (optional)
foreach ($doc in @("README.md","CHANGELOG.md","LICENSE","LICENCE","LICENSE.txt","LICENCE.txt")) {
  $src = Join-Path $PSScriptRoot $doc
  if (Test-Path $src) { Copy-Item -Force $src $DistRoot }
}

# Optionally, install to user bin for global 'uaengine'
if ($InstallToUserBin) {
  $UserBin = Join-Path $env:USERPROFILE 'bin'
  New-Item -ItemType Directory -Force -Path $UserBin | Out-Null
  Copy-Item -Force $DstExe (Join-Path $UserBin $ExeName)
  Write-Host "Installed to: $UserBin\$ExeName"
  $UserPath = [Environment]::GetEnvironmentVariable('PATH','User')
  if (-not ($UserPath -split ';' | Where-Object { $_ -eq $UserBin })) {
    Write-Host "NOTE: $UserBin is not in your user PATH. Run install.ps1 first or add it manually."
  }
}

# Optionally zip the dist content
if ($Zip) {
  Add-Type -AssemblyName 'System.IO.Compression.FileSystem'

  $ZipFinal = Join-Path $DistRoot 'uengine-windows.zip'
  # Create in a temp location to avoid including the zip inside itself
  $ZipTemp  = Join-Path ([System.IO.Path]::GetTempPath()) "uengine-windows.zip"

  if (Test-Path $ZipTemp)  { Remove-Item -Force $ZipTemp }
  if (Test-Path $ZipFinal) { Remove-Item -Force $ZipFinal }

  $DistAbs = [System.IO.Path]::GetFullPath($DistRoot)
  [System.IO.Compression.ZipFile]::CreateFromDirectory($DistAbs, $ZipTemp, [System.IO.Compression.CompressionLevel]::Optimal, $false)

  Move-Item -Force $ZipTemp $ZipFinal
  Write-Host "Created: $ZipFinal"
}

# Summary
Write-Host "`nDone."
Write-Host " - Binary: $DstExe"
if ($Zip) { Write-Host " - Zip:    $ZipFinal" }
