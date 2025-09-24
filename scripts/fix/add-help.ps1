<#-----------------------------------------------------------------------------
 Umicom AuthorEngine AI (uaengine)
 File: scripts/fix/add-help.ps1
 PURPOSE: Non-destructive insertion of --help/-h handler into src\main.c

 Created by: Umicom Foundation (https://umicom.foundation/)
 Author: Sammy Hegab
 Date: 24-09-2025
 License: MIT
-----------------------------------------------------------------------------#>

$path = "src\main.c"
if (-not (Test-Path $path)) {
  Write-Error "Not found: $path (run from repo root)"
  exit 1
}

$text = Get-Content $path -Raw

# If the handler already exists, do nothing.
if ($text -match '\bstrcmp\s*\(\s*argv\[1\]\s*,\s*"--help"\s*\)') {
  Write-Host "[info] --help handler already present; no changes made."
  exit 0
}

$insertion = @'
  /* -----------------------------------------------------------------------
   * Accept '--help' / '-h' explicitly and exit 0 (so CI steps never fail
   * when checking the help text). This is additive and does not change any
   * existing behavior. 'uaengine help' remains supported below.
   * Created by: Umicom Foundation (https://umicom.foundation/)
   * Author: Sammy Hegab
   * Date: 24-09-2025
   * --------------------------------------------------------------------- */
  if ((strcmp(argv[1], "--help") == 0) || (strcmp(argv[1], "-h") == 0)) {
    usage();
    return 0;
  }
'@

# Insert right after the --version block's closing '}'.
# We look for the first close brace after the --version check.
$pattern = '(?s)(\bif\s*\(\s*strcmp\s*\(\s*argv\[1\]\s*,\s*"--version"\s*\)\s*==\s*0\s*\)\s*\{.*?\}\s*)'
if ($text -match $pattern) {
  $new = [System.Text.RegularExpressions.Regex]::Replace($text, $pattern, "`$1`r`n$insertion", 1)
  Set-Content -Encoding UTF8 $path $new
  Write-Host "[ok] Inserted --help/-h handler."
} else {
  Write-Warning "Pattern for --version block not found. No changes made."
  exit 2
}