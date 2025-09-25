# ------------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine) - Runner helper (PowerShell)
#
# PURPOSE:
#   - Robustly locates a built 'uaengine(.exe)' in common build folders.
#   - If not found and UENG_RUN_AUTOBUILD=1 is set, configure+build with CMake:
#       * Uses Ninja if available, otherwise default (Visual Studio) generator.
#   - Forwards all remaining arguments to the binary (preserving quoting).
#
# USAGE:
#   powershell -ExecutionPolicy Bypass -File scripts\run-uaengine.ps1 -- --version
#   $env:UENG_RUN_AUTOBUILD="1"; scripts\run-uaengine.ps1 -- llm-selftest gpt-4o-mini
#
# ENV:
#   UENG_RUN_VERBOSE=1    -> extra diagnostics
#   UENG_RUN_AUTOBUILD=1  -> configure+build if 'uaengine' is missing
#   UAENG_CMAKE_FLAGS     -> extra cmake flags, e.g.:
#                            -DUAENG_ENABLE_OPENAI=ON -DUAENG_ENABLE_OLLAMA=ON
#   CMAKE_BUILD_TYPE      -> used when Ninja is selected (default: Release)
#   CC / CXX              -> honored during autobuild (if set)
#
# NOTES:
#   - We resolve the script directory robustly, even if $PSCommandPath is null
#     (older hosts or unusual invocations).
#
# Project notes (paste any legacy header/comments you want to preserve here):
#   - Created by: Umicom Foundation (https://umicom.foundation/)
#   - Author: Sammy Hegab
#   - Date: 25-09-2025
#   - License: MIT
# ------------------------------------------------------------------------------

param(
  [Parameter(ValueFromRemainingArguments = $true)]
  [string[]]$Rest
)

function Log($msg) {
  if ($env:UENG_RUN_VERBOSE -eq "1") { Write-Host "[run] $msg" }
}

# --- Resolve script path & repo root (robustly) --------------------------------
$ScriptPath = $PSCommandPath
if (-not $ScriptPath -or $ScriptPath -eq "") { $ScriptPath = $MyInvocation.MyCommand.Path }
if (-not $ScriptPath -or $ScriptPath -eq "") { $ScriptPath = (Join-Path (Get-Location) "scripts\run-uaengine.ps1") }

$ScriptDir = Split-Path -Path $ScriptPath -Parent
$RepoRoot  = Split-Path -Path $ScriptDir  -Parent

# --- Candidate locations (common build outputs) --------------------------------
$candidates = @(
  (Join-Path $RepoRoot "build\uaengine.exe"),
  (Join-Path $RepoRoot "build\Debug\uaengine.exe"),
  (Join-Path $RepoRoot "build\Release\uaengine.exe"),
  (Join-Path $RepoRoot "build-ninja\uaengine.exe")
)

function Find-Uaengine {
  foreach ($p in $candidates) {
    if (Test-Path $p) { return $p }
  }
  return $null
}

# Split arguments after '--' to avoid PowerShell re-parsing user args.
$dashdashIndex = $Rest.IndexOf('--')
if ($dashdashIndex -ge 0) {
  $pass = $Rest[($dashdashIndex + 1)..($Rest.Count - 1)]
} else {
  $pass = $Rest
}

$exe = Find-Uaengine
if ($exe) {
  Log "Found uaengine: $exe"
  & $exe @pass
  exit $LASTEXITCODE
}

# --- Optional autobuild --------------------------------------------------------
if ($env:UENG_RUN_AUTOBUILD -eq "1") {
  Log "uaengine not found; attempting autobuild…"

  $haveNinja = (Get-Command ninja -ErrorAction SilentlyContinue) -ne $null
  if ($haveNinja) {
    Log "Using Ninja generator"
    $buildDir = Join-Path $RepoRoot "build-ninja"
    $cmakeArgs = @("-S", $RepoRoot, "-B", $buildDir, "-G", "Ninja",
                   "-D", "CMAKE_BUILD_TYPE=$($env:CMAKE_BUILD_TYPE ?? 'Release')")
  } else {
    Log "Ninja not found; using default CMake generator"
    $buildDir = Join-Path $RepoRoot "build"
    $cmakeArgs = @("-S", $RepoRoot, "-B", $buildDir)
  }

  # Honor CC/CXX if provided (CMake expects compilers at configure time)
  if ($env:CC)  { $cmakeArgs += @("-D", "CMAKE_C_COMPILER=$($env:CC)") }
  if ($env:CXX) { $cmakeArgs += @("-D", "CMAKE_CXX_COMPILER=$($env:CXX)") }

  # Append user flags (space-separated)
  if ($env:UAENG_CMAKE_FLAGS) {
    $cmakeArgs += $env:UAENG_CMAKE_FLAGS.Split(' ')
  }

  Log ("cmake " + ($cmakeArgs -join ' '))
  $p = Start-Process cmake -ArgumentList $cmakeArgs -NoNewWindow -PassThru -Wait
  if ($p.ExitCode -ne 0) {
    Write-Error "[run] CMake configure failed."; exit $p.ExitCode
  }

  $p = Start-Process cmake -ArgumentList @("--build", $buildDir, "-j") -NoNewWindow -PassThru -Wait
  if ($p.ExitCode -ne 0) {
    Write-Error "[run] CMake build failed."; exit $p.ExitCode
  }

  $exe = Find-Uaengine
  if ($exe) {
    Log "Autobuild produced: $exe"
    & $exe @pass
    exit $LASTEXITCODE
  }
}

# --- Friendly guidance ---------------------------------------------------------
Write-Host "[run] ERROR: could not find 'uaengine.exe' in expected locations." -ForegroundColor Red
Write-Host "Tried:"; $candidates | ForEach-Object { Write-Host "  $_" }
Write-Host ""
Write-Host "Hints:"
Write-Host "  1) Default (Visual Studio generator):"
Write-Host "       cmake -S . -B build"
Write-Host "       cmake --build build -j"
Write-Host "     Binary is typically: build\Debug\uaengine.exe (or Release)"
Write-Host "  2) Ninja:"
Write-Host "       cmake -S . -B build-ninja -G Ninja -DCMAKE_BUILD_TYPE=Release"
Write-Host "       cmake --build build-ninja -j"
exit 1
