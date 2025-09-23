param(
  [Parameter(Mandatory=$true)]
  [string]$Path
)

if (-not (Test-Path $Path)) {
  Write-Error "File not found: $Path"
  exit 1
}

$hash = Get-FileHash -Algorithm SHA256 -Path $Path
Write-Host "SHA256  $($hash.Hash)  $Path"
