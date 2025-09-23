# build.ps1 — configure & build with CMake, then pack and install
param(
  [string]$BuildDir = ".\build",
  [ValidateSet('Release','Debug','RelWithDebInfo','MinSizeRel')]
  [string]$BuildType = "Release",
  [ValidateSet('Ninja','Visual Studio 17 2022')]
  [string]$Generator = "Ninja",
  [switch]$Clean,
  [switch]$Zip,                 # forward to pack.ps1 to create dist\uengine-windows.zip
  [string]$CCompiler            # optional: path to C compiler (e.g., 'C:\Program Files\LLVM\bin\clang.exe')
)

$ErrorActionPreference = "Stop"
$root = $PSScriptRoot

function Exec($cmd, [string[]]$argList) {
  Write-Host ">> $cmd $($argList -join ' ')"
  & $cmd @argList
  if ($LASTEXITCODE -ne 0) { throw "Command failed ($cmd): exit $LASTEXITCODE" }
}

# 1) Clean build dir if requested
if ($Clean -and (Test-Path $BuildDir)) {
  Write-Host "Cleaning build directory: $BuildDir"
  Remove-Item -Recurse -Force $BuildDir
}

# 2) Configure
$cmakeArgs = @("-S",".","-B",$BuildDir,"-G",$Generator)
if ($Generator -ne "Visual Studio 17 2022") {
  $cmakeArgs += @("-DCMAKE_BUILD_TYPE=$BuildType")
} else {
  $cmakeArgs += @("-A","x64")
}
if ($CCompiler) { $cmakeArgs += @("-DCMAKE_C_COMPILER=$CCompiler") }

Exec "cmake" $cmakeArgs

# 3) Build
if ($Generator -eq "Visual Studio 17 2022") {
  Exec "cmake" @("--build",$BuildDir,"--config",$BuildType,"-j")
} else {
  Exec "cmake" @("--build",$BuildDir,"-j")
}

# 4) Pack & (optionally) install to user bin
$pack = Join-Path $root "pack.ps1"
if (-not (Test-Path $pack)) { throw "Missing pack.ps1 next to build.ps1 ($pack)" }

$packArgs = @("-ExecutionPolicy","Bypass","-File",$pack,"-BuildDir",$BuildDir,"-InstallToUserBin")
if ($Zip) { $packArgs += "-Zip" }

Write-Host ">> powershell $($packArgs -join ' ')"
& powershell @packArgs
if ($LASTEXITCODE -ne 0) { throw "Packaging failed: exit $LASTEXITCODE" }

Write-Host "`nBuild complete."
