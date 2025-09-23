# tools\make.ps1 — tiny task runner for uaengine
param(
  [Parameter(Position=0)]
  [ValidateSet('build','clean','pack','zip','install','uninstall','vsbuild','release')]
  [string]$Task = 'build',
  [Parameter(Position=1)]
  [string]$Arg
)

$ErrorActionPreference = "Stop"

function Run($file, [string[]]$moreArgs) {
  $full = Join-Path $PSScriptRoot $file
  if (-not (Test-Path $full)) { throw "Not found: $full" }
  $argList = @('-ExecutionPolicy','Bypass','-File', $full) + $moreArgs
  Write-Host ">> powershell $($argList -join ' ')"
  & powershell @argList
  if ($LASTEXITCODE -ne 0) { throw "$file failed with exit code $LASTEXITCODE" }
}

switch ($Task) {
  # Ninja (default) build path
  'build'     { Run 'build.ps1' @() }
  'clean'     { Run 'build.ps1' @('-Clean') }

  # Packaging helpers
  'pack'      { Run 'pack.ps1'  @() }
  'zip'       { Run 'pack.ps1'  @('-Zip') }
  'install'   { Run 'pack.ps1'  @('-InstallToUserBin') }
  'uninstall' { Run 'uninstall.ps1' @() }

  # Visual Studio build uses a separate build dir to avoid generator clashes
  'vsbuild'   { Run 'build.ps1' @('-Generator','Visual Studio 17 2022','-BuildType','Release','-BuildDir','.\\build-vs') }

  # Tag a release at current HEAD (assumes include\ueng\version.h updated)
  'release' {
    if (-not $Arg) { throw "Usage: .\tools\make.ps1 release vX.Y.Z" }
    git add .\include\ueng\version.h | Out-Null
    git commit -m "version: bump to $Arg" 2>$null | Out-Null
    git push origin main
    git tag -a $Arg -m "Release $Arg"
    git push origin $Arg
    Write-Host "Tagged and pushed $Arg"
  }
}
