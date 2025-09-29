<#
    =======================================================================
    Umicom Build Helper - wires vcpkg libcurl & builds the project
    Author: Sammy Hegab
    Organization: Umicom Foundation (https://umicom.foundation/)
    License: MIT
    Date: 2025-09-29

    PURPOSE
    - Ensure `cmake/umicom-helpers.cmake` is included from CMakeLists.txt.
    - Configure CMake with vcpkg toolchain.
    - Build the convenience target `uengine`, which depends on `uaengine`.

    USAGE
      # Visual Studio generator (default), Debug config:
      .\build.ps1

      # Ninja generator:
      .\build.ps1 -UseNinja

      # Release config:
      .\build.ps1 -Config Release

      # Clean build dir first:
      .\build.ps1 -Clean

    PREREQUISITES
      - Visual Studio 2022 C++ Build Tools OR Ninja + MSVC toolchain
      - vcpkg installed at C:\Dev\vcpkg (or set $Env:VCPKG_ROOT)
      - Packages:
          C:\Dev\vcpkg\vcpkg integrate install
          C:\Dev\vcpkg\vcpkg install curl[ssl]:x64-windows
    =======================================================================
    #>

    [CmdletBinding()]
    param(
      [ValidateSet('Debug','Release','RelWithDebInfo','MinSizeRel')]
      [string]$Config = 'Debug',

      [switch]$UseNinja,

      [switch]$Clean,

      [string]$Triplet = 'x64-windows',

      [string]$VcpkgRoot = $(if ($Env:VCPKG_ROOT) { $Env:VCPKG_ROOT } else { 'C:\Dev\vcpkg' })
    )

    $ErrorActionPreference = 'Stop'

    # Resolve paths
    $RepoRoot   = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
    $BuildDir   = Join-Path $RepoRoot 'build'
    $CMakeList  = Join-Path $RepoRoot 'CMakeLists.txt'
    $HelpersRel = 'cmake/umicom-helpers.cmake'
    $HelpersAbs = Join-Path $RepoRoot $HelpersRel

    if (-not (Test-Path $CMakeList)) {
      throw "CMakeLists.txt not found at $RepoRoot. Please run this script from a checkout of umicom-authorengine-ai."
    }

    # Ensure helpers file is present (in case the user only copied the script)
    if (-not (Test-Path $HelpersAbs)) {
      New-Item -ItemType Directory -Force -Path (Split-Path $HelpersAbs) | Out-Null
      @"
# See file committed in tools pack; this placeholder will be overwritten by the real one if you copy it.
include_guard(GLOBAL)
message(STATUS "umicom-helpers placeholder included.")
"@ | Set-Content -NoNewline -Path $HelpersAbs -Encoding UTF8
    }

    # Ensure CMakeLists.txt includes the helpers (idempotent)
    $includeLine = 'include(cmake/umicom-helpers.cmake)'
    $cmakeText = Get-Content -Path $CMakeList -Raw
    if ($cmakeText -notmatch [regex]::Escape($includeLine)) {
      Add-Content -Path $CMakeList -Value "`n# Umicom Foundation: wire libcurl + uengine target`n$includeLine`n"
      Write-Host "Appended include line to CMakeLists.txt" -ForegroundColor Yellow
    }

    # Toolchain from vcpkg
    $Toolchain = Join-Path $VcpkgRoot 'scripts/buildsystems/vcpkg.cmake'
    if (-not (Test-Path $Toolchain)) {
      throw "vcpkg toolchain file not found at $Toolchain. Make sure vcpkg is installed."
    }

    # Clean build?
    if ($Clean -and (Test-Path $BuildDir)) {
      Write-Host "Removing build directory: $BuildDir" -ForegroundColor Yellow
      Remove-Item -Recurse -Force $BuildDir
    }
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

    # Configure
    $common = @(
      "-S", $RepoRoot,
      "-B", $BuildDir,
      "-DCMAKE_TOOLCHAIN_FILE=$Toolchain",
      "-DVCPKG_TARGET_TRIPLET=$Triplet"
    )

    if ($UseNinja) {
      $gen = @("-G","Ninja")
    } else {
      $gen = @("-G","Visual Studio 17 2022","-A","x64")
    }

    Write-Host "`n== Configuring CMake ==" -ForegroundColor Cyan
    & cmake @gen @common

    # Build
    Write-Host "`n== Building target 'uengine' (which delegates to 'uaengine') ==" -ForegroundColor Cyan

    $buildArgs = @("--build", $BuildDir, "--target", "uengine")
    if (-not $UseNinja) {
      $buildArgs += @("--config", $Config)
    }
    & cmake @buildArgs

    Write-Host "`nDone. You can also open the generated solution under '$BuildDir' in Visual Studio." -ForegroundColor Green
