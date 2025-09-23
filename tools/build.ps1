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
  $joined = $argList -join ' '
  Write-Host ">> $cmd $joined"
  $p = Start-Process -FilePath $cmd -ArgumentList $argList -NoNewWindow -Wait -PassThru
  if ($p.ExitCode -ne 0) { throw "Command failed ($cmd): exit $($p.ExitCode)" }
}

# 1) Clean build dir if requested
if ($Clean -and (Test-Path $BuildDir)) {
  Write-Host "Cleaning build directory: $BuildDir"
  Remove-Item -Recurse -Force $BuildDir
}

# 2) Configure
$cmakeArgs = @("-S", ".", "-B", $BuildDir, "-G", $Generator)
if ($Generator -ne "Visual Studio 17 2022") {
  $cmakeArgs += @("-DCMAKE_BUILD_TYPE=$BuildType")
} else {
  # Visual Studio is multi-config; still accept BuildType for --config during build
  $cmakeArgs += @("-A", "x64")
}
if ($CCompiler) { $cmakeArgs += @("-DCMAKE_C_COMPILER=$CCompiler") }

Exec "cmake" $cmakeArgs

# 3) Build
if ($Generator -eq "Visual Studio 17 2022") {
  Exec "cmake" @("--build", $BuildDir, "--config", $BuildType, "-j")
} else {
  Exec "cmake" @("--build", $BuildDir, "-j")
}

# 4) Pack & (optionally) install to user bin
$pack = Join-Path $root "pack.ps1"
if (-not (Test-Path $pack)) { throw "Missing pack.ps1 next to build.ps1 ($pack)" }

$packArgs = @("-ExecutionPolicy","Bypass","-File", $pack, "-BuildDir", $BuildDir, "-InstallToUserBin")
if ($Zip) { $packArgs += "-Zip" }

Write-Host ">> powershell $($packArgs -join ' ')"
$pp = Start-Process -FilePath "powershell" -ArgumentList $packArgs -NoNewWindow -Wait -PassThru
if ($pp.ExitCode -ne 0) { throw "Packaging failed: exit $($pp.ExitCode)" }

Write-Host "`nBuild complete."
