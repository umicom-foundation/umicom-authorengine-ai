param(
  [string]$Version = "",
  [string]$WindowsUrl = "",
  [string]$MacUrl = "",
  [string]$LinuxUrl = ""
)

$ErrorActionPreference = "Stop"

function Get-Hash([string]$Path) {
  if (-not (Test-Path $Path)) { throw "File not found: $Path" }
  $h = Get-FileHash -Algorithm SHA256 -Path $Path
  return $h.Hash
}

# Build URLs from version if explicit URLs weren't provided
if ($Version -and -not $WindowsUrl) { $WindowsUrl = "https://github.com/umicom-foundation/umicom-authorengine-ai/releases/download/$Version/uaengine-windows.zip" }
if ($Version -and -not $MacUrl)     { $MacUrl     = "https://github.com/umicom-foundation/umicom-authorengine-ai/releases/download/$Version/uaengine-macos.zip" }
if ($Version -and -not $LinuxUrl)   { $LinuxUrl   = "https://github.com/umicom-foundation/umicom-authorengine-ai/releases/download/$Version/uaengine-linux.zip" }

$temp = Join-Path $env:TEMP ("uaeng-hashes-" + [guid]::NewGuid())
New-Item -ItemType Directory -Force -Path $temp | Out-Null

$results = @()

try {
  if ($WindowsUrl) {
    $winPath = Join-Path $temp "uaengine-windows.zip"
    Write-Host "Downloading Windows zip from: $WindowsUrl"
    Invoke-WebRequest -Uri $WindowsUrl -OutFile $winPath -UseBasicParsing
    $results += [pscustomobject]@{ OS="Windows"; Path=$winPath; SHA256=(Get-Hash $winPath) }
  }

  if ($MacUrl) {
    $macPath = Join-Path $temp "uaengine-macos.zip"
    Write-Host "Downloading macOS zip from: $MacUrl"
    Invoke-WebRequest -Uri $MacUrl -OutFile $macPath -UseBasicParsing
    $results += [pscustomobject]@{ OS="macOS"; Path=$macPath; SHA256=(Get-Hash $macPath) }
  }

  if ($LinuxUrl) {
    $linPath = Join-Path $temp "uaengine-linux.zip"
    Write-Host "Downloading Linux zip from: $LinuxUrl"
    Invoke-WebRequest -Uri $LinuxUrl -OutFile $linPath -UseBasicParsing
    $results += [pscustomobject]@{ OS="Linux"; Path=$linPath; SHA256=(Get-Hash $linPath) }
  }

  if ($results.Count -eq 0) {
    Write-Warning "No URLs provided and no version specified; nothing to do."
    exit 1
  }

  "`nSHA256 hashes:"
  $results | ForEach-Object {
    "{0,-7}  {1}" -f $_.OS, $_.SHA256
  } | Out-Host

  "`nUse these values in:"
  " - packaging\uaengine.json (Scoop): ""hash"": ""<SHA256 of Windows zip>"""
  " - packaging\uaengine.rb (Homebrew): sha256 ""<SHA256 of macOS zip>"" and ""<SHA256 of Linux zip>"""
}
finally {
  # Leave the files for auditing; comment the next line if you want to keep them
  # Remove-Item -Recurse -Force $temp
}
