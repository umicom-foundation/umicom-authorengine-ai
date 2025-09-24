# PowerShell packaging script for Windows
$ErrorActionPreference = "Stop"
$App = "uaengine.exe"
$Build = "build"
$Out = "dist"
if (!(Test-Path $Out)) { New-Item -ItemType Directory -Path $Out | Out-Null }
$ver = & ".\$Build\uaengine.exe" --version
$ver = $ver -replace ' ', '_'

$zipPath = ".\$Out\uaengine-$ver-windows-x64.zip"
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path ".\$Build\$App" -DestinationPath $zipPath
Write-Host "Created $zipPath"
