# tools\make.ps1 — tiny task runner for uaengine
param(
  [Parameter(Position=0)]
  [ValidateSet('build','clean','pack','zip','install','uninstall','vsbuild','release')]
  [string]$Task = 'build',
  [Parameter(Position=1)]
  [string]$Arg
)

$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot\..").Path

function Run($file, [string[]]$args) {
  $full = Join-Path $PSScriptRoot $file
  if (-not (Test-Path $full)) { throw "Not found: $full" }
  Write-Host ">> powershell -ExecutionPolicy Bypass -File $file $($args -join ' ')"
  $p = Start-Process -FilePath "powershell" -ArgumentList @("-ExecutionPolicy","Bypass","-File",$full)+$args -NoNewWindow -Wait -PassThru
  if ($p.ExitCode -ne 0) { throw "$file failed with exit code $($p.ExitCode)" }
}

switch ($Task) {
  'build'     { Run 'build.ps1' @() }
  'clean'     { Run 'build.ps1' @('-Clean') }
  'pack'      { Run 'pack.ps1'  @() }
  'zip'       { Run 'pack.ps1'  @('-Zip') }
  'install'   { Run 'pack.ps1'  @('-InstallToUserBin') }
  'uninstall' { & (Join-Path $PSScriptRoot 'uninstall.ps1') }
  'vsbuild'   { Run 'build.ps1' @('-Generator','"Visual Studio 17 2022"','-BuildType','Release') }
  'release' {
    if (-not $Arg) { throw "Usage: .\tools\make.ps1 release vX.Y.Z" }
    # Ensure version header matches (optional manual step)
    git add .\include\ueng\version.h
    git commit -m "version: bump to $Arg" | Out-Null
    git push origin main
    git tag -a $Arg -m "Release $Arg"
    git push origin $Arg
    Write-Host "Tagged and pushed $Arg"
  }
}
