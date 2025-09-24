# PowerShell smoke test
$exe = Join-Path (Resolve-Path ".\build").Path "uaengine.exe"
if (-Not (Test-Path $exe)) {
  Write-Host "uaengine.exe not found in .\build. Did you build it?" -ForegroundColor Yellow
  exit 0
}
& $exe --help
& $exe --version
