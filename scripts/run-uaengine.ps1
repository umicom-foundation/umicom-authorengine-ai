#------------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# File: scripts/run-uaengine.ps1
# PURPOSE: Cross-platform-friendly PowerShell launcher to find and run the
#          built 'uaengine' binary from common build directories.
#
# Created by: Umicom Foundation (https://umicom.foundation/)
# Author: Sammy Hegab
# Date: 25-09-2025
# License: MIT
#------------------------------------------------------------------------------
<#
.SYNOPSIS
  Launches the compiled 'uaengine' binary regardless of whether you built with
  Visual Studio (MSBuild) or Ninja.

.DESCRIPTION
  This script looks for the 'uaengine(.exe)' binary in several common output
  folders relative to the repository root (the folder that contains this
  'scripts' directory). The first match is executed and all arguments you pass
  to this script are forwarded to the executable unchanged.

  Why this exists:
    - New contributors shouldn't need to remember whether they built with
      Ninja or MSBuild, or which configuration was last used.
    - CI and documentation can reference a single stable entry point.
    - The script prints friendly hints if the binary is missing.

.PARAMETER (passthrough via $args)
  Any arguments after the script name are forwarded to 'uaengine'. For example:
    powershell -ExecutionPolicy Bypass -File scripts\run-uaengine.ps1 -- --version
    powershell -ExecutionPolicy Bypass -File scripts\run-uaengine.ps1 -- llm-selftest gpt-4o-mini

.ENVIRONMENT
  UENG_RUN_VERBOSE=1   -> prints extra diagnostics about resolution.

.NOTES
  The script is intentionally dependency-free and uses only built-in PowerShell.
#>

# Determine repo root: this script lives at <repo>/scripts/run-uaengine.ps1
$ScriptDir = Split-Path -LiteralPath $PSCommandPath -Parent
$RepoRoot  = Split-Path -LiteralPath $ScriptDir -Parent

# Helper to write verbose messages only when requested
function Write-RunVerbose([string]$msg) { if ($env:UENG_RUN_VERBOSE) { Write-Host "[run] $msg" } }

# Candidates to probe (first existing wins). We probe both .exe and bare name, just in case.
$candidates = @(
  "build/uaengine.exe",
  "build/uaengine",
  "build/Release/uaengine.exe",
  "build/Debug/uaengine.exe",
  "build-ninja/uaengine.exe",
  "build-ninja/uaengine"
) | ForEach-Object { Join-Path $RepoRoot $_ }

$Resolved = $null
foreach ($c in $candidates) {
  Write-RunVerbose "checking $c"
  if (Test-Path -LiteralPath $c) { $Resolved = $c; break }
}

if (-not $Resolved) {
  Write-Host "[run] ERROR: could not find 'uaengine' in expected build folders." -ForegroundColor Red
  Write-Host "Tried:"
  $candidates | ForEach-Object { Write-Host "  - $_" }
  Write-Host ""
  Write-Host "Hints:"
  Write-Host "  1) Build with MSBuild (Visual Studio generator):"
  Write-Host "       cmake -S . -B build"
  Write-Host "       cmake --build build -j"
  Write-Host "     The binary will be in 'build/Debug/uaengine.exe' (or Release)."
  Write-Host ""
  Write-Host "  2) Build with Ninja:"
  Write-Host "       cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release"
  Write-Host "       cmake --build build-ninja -j"
  Write-Host "     The binary will be in 'build-ninja/uaengine.exe'."
  exit 1
}

Write-RunVerbose "resolved to $Resolved"
# Forward all arguments verbatim to the binary
& $Resolved @args
$LASTEXITCODE
