# uninstall.ps1 — remove installed uaengine.exe from %USERPROFILE%\bin and clean PATH entry if empty

$Bin = Join-Path $env:USERPROFILE 'bin'
$Exe = Join-Path $Bin 'uaengine.exe'

Write-Host "Uninstalling uaengine from: $Exe"

# 1) Remove the installed binary, if present
if (Test-Path $Exe) {
  try {
    Remove-Item -Force $Exe
    Write-Host "Removed: $Exe"
  } catch {
    Write-Error "Failed to remove $Exe : $_"
    exit 1
  }
} else {
  Write-Host "Nothing to remove at $Exe (file not found)."
}

# 2) If the bin folder is now empty, remove it from the *User* PATH
function Get-UserPath { [Environment]::GetEnvironmentVariable('PATH', 'User') }
function Set-UserPath($value) { [Environment]::SetEnvironmentVariable('PATH', $value, 'User') }

$IsEmpty = $false
if (Test-Path $Bin) {
  $items = Get-ChildItem -Force -Path $Bin | Where-Object { $_.Name -notin @('.', '..') }
  $IsEmpty = ($items.Count -eq 0)
}

$UserPath = Get-UserPath

if ($IsEmpty) {
  $parts = @()
  if ($UserPath) { $parts = $UserPath.Split(';') | Where-Object { $_ -ne '' } }
  if ($parts -contains $Bin) {
    $NewPath = ($parts | Where-Object { $_ -ne $Bin }) -join ';'
    Set-UserPath $NewPath
    Write-Host "Removed $Bin from your USER PATH because it is empty."
    Write-Host "NOTE: Open a NEW terminal window for the PATH change to take effect."
  } else {
    Write-Host "$Bin was not in USER PATH (no change)."
  }
} else {
  Write-Host "$Bin is not empty; leaving USER PATH unchanged."
}

# 3) Session PATH cleanup (optional)
# If current session PATH starts with $Bin; remove it for this session only
$sessionParts = $env:PATH.Split(';') | Where-Object { $_ -ne '' }
if ($sessionParts[0] -eq $Bin) {
  $env:PATH = ($sessionParts | Where-Object { $_ -ne $Bin }) -join ';'
  Write-Host "Updated current session PATH to remove leading $Bin."
}

# 4) Show what 'uaengine' resolves to now (if any)
$resolved = Get-Command uaengine -ErrorAction SilentlyContinue
if ($resolved) {
  Write-Host "Current 'uaengine' resolves to: $($resolved.Path)"
} else {
  Write-Host "'uaengine' is no longer on PATH (expected after uninstall)."
}
