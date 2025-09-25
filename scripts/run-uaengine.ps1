<# -----------------------------------------------------------------------------
 Umicom AuthorEngine AI (uaengine) - Windows runner helper (PowerShell)

 PURPOSE:
   - Robustly locates a built 'uaengine(.exe)' in common build folders.
   - If not found and UENG_RUN_AUTOBUILD=1 is set, it will configure+build
     using either Ninja (if available) or the default MSBuild generator.
   - Passes all remaining arguments through to the binary.

 USAGE:
   powershell -ExecutionPolicy Bypass -File scripts\run-uaengine.ps1 -- --version
   powershell -ExecutionPolicy Bypass -File scripts\run-uaengine.ps1 -- llm-selftest gpt-4o-mini

 ENV:
   UENG_RUN_VERBOSE=1    -> prints extra diagnostics
   UENG_RUN_AUTOBUILD=1  -> tries to build if the binary is missing

 Created by: Umicom Foundation (https://umicom.foundation/)
 Author: Sammy Hegab
 Date: 25-09-2025
 License: MIT
----------------------------------------------------------------------------- #>

param(
  [Parameter(ValueFromRemainingArguments=$true)]
  [string[]] $Args
)

# ------------------------------ Helpers ---------------------------------------
function Write-Info($msg)  { if ($env:UENG_RUN_VERBOSE) { Write-Host "[run] $msg" } }
function Write-ErrorBlock($title, $lines) {
  Write-Host "[run] ERROR: $title" -ForegroundColor Red
  if ($lines) { $lines | ForEach-Object { Write-Host $_ } }
}

# Resolve script directory in a way that works even when PSCommandPath is null.
$ScriptDir = $PSScriptRoot
if (-not $ScriptDir -or $ScriptDir -eq '') {
  $ScriptDir = Split-Path -Path $MyInvocation.MyCommand.Path -Parent
}
$RepoRoot = Split-Path -Path $ScriptDir -Parent

# Candidate locations (Debug/Release/Ninja/single-config).
$candidates = @(
  (Join-Path $RepoRoot "build\uaengine.exe"),
  (Join-Path $RepoRoot "build\Debug\uaengine.exe"),
  (Join-Path $RepoRoot "build\Release\uaengine.exe"),
  (Join-Path $RepoRoot "build-ninja\uaengine.exe")
)

function Find-Uaengine {
  foreach ($c in $candidates) {
    if (Test-Path $c) { return $c }
  }
  return $null
}

# Try to find an existing binary.
$exe = Find-Uaengine
if ($exe) {
  Write-Info "Found uaengine: $exe"
  & $exe @Args
  exit $LASTEXITCODE
}

# Optionally attempt to (auto)build if requested by env flag.
if ($env:UENG_RUN_AUTOBUILD -eq '1') {
  Write-Info "uaengine not found; attempting autobuild..."

  # Prefer Ninja if available.
  $ninja = $null
  try { $ninja = (Get-Command ninja.exe -ErrorAction Stop).Source } catch {}

  if ($ninja) {
    Write-Info "Using Ninja at: $ninja"
    # If LLVM/Clang is present, use it; otherwise let CMake default to MSVC.
    $clang   = "C:\Program Files\LLVM\bin\clang.exe"
    $clangxx = "C:\Program Files\LLVM\bin\clang++.exe"

    if (Test-Path $clang -and Test-Path $clangxx) {
      & cmake -S $RepoRoot -B (Join-Path $RepoRoot "build-ninja") -G Ninja `
        -D CMAKE_BUILD_TYPE=Release `
        -D CMAKE_MAKE_PROGRAM="$ninja" `
        -D CMAKE_C_COMPILER="$clang" `
        -D CMAKE_CXX_COMPILER="$clangxx"
    } else {
      & cmake -S $RepoRoot -B (Join-Path $RepoRoot "build-ninja") -G Ninja `
        -D CMAKE_BUILD_TYPE=Release `
        -D CMAKE_MAKE_PROGRAM="$ninja"
    }
    if ($LASTEXITCODE -ne 0) { Write-ErrorBlock "CMake configure (Ninja) failed." $null; exit 1 }
    & cmake --build (Join-Path $RepoRoot "build-ninja") -j
    if ($LASTEXITCODE -ne 0) { Write-ErrorBlock "CMake build (Ninja) failed." $null; exit 1 }
  }
  else {
    Write-Info "Ninja not found; using MSBuild generator."
    & cmake -S $RepoRoot -B (Join-Path $RepoRoot "build")
    if ($LASTEXITCODE -ne 0) { Write-ErrorBlock "CMake configure (MSBuild) failed." $null; exit 1 }
    & cmake --build (Join-Path $RepoRoot "build") -j
    if ($LASTEXITCODE -ne 0) { Write-ErrorBlock "CMake build (MSBuild) failed." $null; exit 1 }
  }

  # Try again to resolve the binary.
  $exe = Find-Uaengine
  if ($exe) {
    Write-Info "Autobuild produced: $exe"
    & $exe @Args
    exit $LASTEXITCODE
  }
}

# If we’re still here, we couldn’t find (or build) the binary.
Write-ErrorBlock "could not find 'uaengine' in expected build folders." @(
  "Tried:", ("  " + ($candidates -join "`n  ")), "",
  "Hints:",
  "  1) Build with MSBuild (Visual Studio generator):",
  "       cmake -S . -B build",
  "       cmake --build build -j",
  "     The binary will be in 'build\\Debug\\uaengine.exe' (or Release).",
  "",
  "  2) Build with Ninja:",
  "       cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release",
  "       cmake --build build-ninja -j",
  "     The binary will be in 'build-ninja\\uaengine.exe'."
)
exit 1
# End of script