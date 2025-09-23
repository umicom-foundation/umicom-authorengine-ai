# install.ps1 — copy the latest uaengine.exe to a user bin dir and add it to PATH

$Bin = Join-Path $env:USERPROFILE 'bin'
New-Item -ItemType Directory -Force -Path $Bin | Out-Null

# Source exe from the Ninja build folder (adjust if you use a different build dir)
$Src = Join-Path $PSScriptRoot 'build\uaengine.exe'
if (-not (Test-Path $Src)) {
  Write-Error "Not found: $Src. Build first: cmake -S . -B build -G Ninja; cmake --build build"
  exit 1
}

$Dst = Join-Path $Bin 'uaengine.exe'
Copy-Item -Force $Src $Dst
Write-Host "Installed: $Dst"

# Ensure user-level PATH includes %USERPROFILE%\bin
$Current = [Environment]::GetEnvironmentVariable('PATH', 'User')
$Parts = @()
if ($Current) { $Parts = $Current.Split(';') | Where-Object { $_ -ne '' } }

if ($Parts -notcontains $Bin) {
  $NewPath = if ($Current -and $Current.Trim()) { "$Current;$Bin" } else { $Bin }
  [Environment]::SetEnvironmentVariable('PATH', $NewPath, 'User')
  Write-Host "Added $Bin to your user PATH."
  Write-Host "NOTE: Open a NEW terminal window for the PATH change to take effect."
} else {
  Write-Host "$Bin already in your user PATH."
}

# Optional: warn if an older uaengine is ahead in PATH
$Resolved = (Get-Command uaengine -ErrorAction SilentlyContinue)
if ($Resolved) {
  Write-Host "Current 'uaengine' resolves to: $($Resolved.Path)"
  if ($Resolved.Path -ne $Dst) {
    Write-Warning "Another uaengine is taking precedence. After opening a NEW terminal, it should resolve to $Dst."
  }
} else {
  Write-Host "After opening a NEW terminal, 'uaengine --version' should work globally."
}
