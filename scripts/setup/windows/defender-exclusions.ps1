# Exclude heavy dev folders (Admin)
$paths = @("D:\Dev","D:\Build","D:\Models","D:\Data","D:\Caches")
foreach ($p in $paths) {
  if (-not (Test-Path $p)) { New-Item -ItemType Directory -Path $p -Force | Out-Null }
  Add-MpPreference -ExclusionPath $p -ErrorAction SilentlyContinue
  Write-Host "Excluded $p" -ForegroundColor Green
}
