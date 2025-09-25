<#-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * File: scripts/run-uaengine.ps1
 * PURPOSE: Cross-platform runner helper for locating and executing the built
 *          'uaengine' binary on Windows. It discovers common MSBuild and
 *          Ninja output folders and forwards all arguments to the binary.
 *
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab
 * Date: 24-09-2025
 * License: MIT
 *---------------------------------------------------------------------------#>

#------------------------------- HOW IT WORKS --------------------------------#
# - We first discover the path to THIS script, even if invoked in unusual
#   ways (e.g., -File, dot-sourced, or via another working directory).
# - From the script path, we compute the repo root (parent dir), or fall back
#   to 'git rev-parse --show-toplevel'.
# - We then probe several candidate paths where 'uaengine(.exe)' usually is:
#     build/uaengine.exe
#     build/Debug/uaengine.exe
#     build/Release/uaengine.exe
#     build-ninja/uaengine.exe
#     (plus the same paths relative to the current directory as a fallback)
# - The first one that exists is executed with all user-passed args.
# - Set UENG_RUN_VERBOSE=1 to see the probing list.
#----------------------------------------------------------------------------#

# Make errors fail the script immediately
$ErrorActionPreference = 'Stop'

# 1) Discover the full path to this script robustly
#    - $MyInvocation.MyCommand.Path works across PowerShell versions,
#      even when $PSCommandPath is not set.
$ScriptPath = $null
try { $ScriptPath = $MyInvocation.MyCommand.Path } catch {}
if (-not $ScriptPath) {
  try { $ScriptPath = $PSCommandPath } catch {}
}

# If we *still* don't have a script path, try to infer it from git or CWD.
if (-not $ScriptPath) {
  # Best effort: assume the script lives at scripts/run-uaengine.ps1 under repo root
  $gitRoot = $(git rev-parse --show-toplevel 2>$null)
  if ($gitRoot) {
    $ScriptPath = Join-Path $gitRoot 'scripts\run-uaengine.ps1'
  } else {
    # Last-ditch: current directory
    $ScriptPath = Join-Path (Get-Location) 'scripts\run-uaengine.ps1'
  }
}

# 2) Compute script dir and repo root (parent of scripts/)
$ScriptDir = Split-Path -Path $ScriptPath -Parent
$RepoRoot  = Split-Path -Path $ScriptDir  -Parent

# If we couldn't derive a repo root (very unusual), try git again
if (-not $RepoRoot -or -not (Test-Path $RepoRoot)) {
  $gitRoot = $(git rev-parse --show-toplevel 2>$null)
  if ($gitRoot) { $RepoRoot = $gitRoot }
}

# Helper: add a candidate path if it’s not null/empty
function Add-Candidate([System.Collections.Generic.List[string]]$list, [string]$p) {
  if ([string]::IsNullOrWhiteSpace($p)) { return }
  $list.Add($p) | Out-Null
}

# 3) Build candidate list
$c = [System.Collections.Generic.List[string]]::new()

# Preferred (relative to repo root)
Add-Candidate $c (Join-Path $RepoRoot 'build\uaengine.exe')
Add-Candidate $c (Join-Path $RepoRoot 'build\Debug\uaengine.exe')
Add-Candidate $c (Join-Path $RepoRoot 'build\Release\uaengine.exe')
Add-Candidate $c (Join-Path $RepoRoot 'build-ninja\uaengine.exe')

# Fallbacks (relative to current working directory)
Add-Candidate $c 'build\uaengine.exe'
Add-Candidate $c 'build\Debug\uaengine.exe'
Add-Candidate $c 'build\Release\uaengine.exe'
Add-Candidate $c 'build-ninja\uaengine.exe'

if ($env:UENG_RUN_VERBOSE) {
  Write-Host "[run] Probing for 'uaengine' in:"
  $c | ForEach-Object { Write-Host "  - $_" }
}

# 4) Pick the first that exists
$Exe = $null
foreach ($p in $c) {
  if (Test-Path -LiteralPath $p) { $Exe = (Resolve-Path -LiteralPath $p).Path; break }
}

if (-not $Exe) {
  Write-Host "[run] ERROR: could not find 'uaengine' in expected build folders." -ForegroundColor Red
  Write-Host "Tried:" -ForegroundColor Yellow
  $c | ForEach-Object { Write-Host "  $_" }
  Write-Host "`nHints:" -ForegroundColor Yellow
  Write-Host "  1) Build with MSBuild (Visual Studio generator):"
  Write-Host "       cmake -S . -B build"
  Write-Host "       cmake --build build -j"
  Write-Host "     The binary will be in 'build\Debug\uaengine.exe' (or Release)."
  Write-Host "`n  2) Build with Ninja:"
  Write-Host "       cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release"
  Write-Host "       cmake --build build-ninja -j"
  Write-Host "     The binary will be in 'build-ninja\uaengine.exe'."
  exit 1
}

if ($env:UENG_RUN_VERBOSE) { Write-Host "[run] Using: $Exe" -ForegroundColor Green }

# 5) Forward all arguments after '--' (or all args if none)
#    We accept both:
#      run-uaengine.ps1 -- --version
#      run-uaengine.ps1 --version
$argsToPass = $args
if ($args.Length -ge 1 -and $args[0] -eq '--') {
  $argsToPass = $args[1..($args.Length-1)]
}

# Exec
& $Exe @argsToPass
exit $LASTEXITCODE
