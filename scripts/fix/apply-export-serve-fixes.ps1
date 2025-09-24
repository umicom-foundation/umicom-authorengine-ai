# -----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# PURPOSE: In-place fixes for main.c (pandoc quoting) and serve.c ('\0' clamp)
# Created by: Umicom Foundation | Developer: Sammy Hegab | Date: 24-09-2025 | MIT
# -----------------------------------------------------------------------------
$ErrorActionPreference = "Stop"

$main = "src\main.c"
if (Test-Path $main) {
  $c = Get-Content $main -Raw
  $c2 = $c `
    -replace '""workspace/book-draft\.md""','"\"workspace/book-draft.md\""' `
    -replace '(^\s*)\\\"workspace/book-draft\.md\\\"(\s*,\s*)', '$1"\"workspace/book-draft.md\""${2}'
  if ($c2 -ne $c) { Set-Content -Path $main -Value $c2 -Encoding UTF8; Write-Host "[fix] main.c quoting adjusted" -ForegroundColor Green }
  else { Write-Host "[info] main.c already OK or pattern not found" -ForegroundColor Yellow }
} else { Write-Host "[warn] src\main.c not found" -ForegroundColor Yellow }

$serve = "src\serve.c"
if (Test-Path $serve) {
  $s = Get-Content $serve -Raw
  $s2 = $s -replace "'<U\+0000>'","'\0'"
  if ($s2 -ne $s) { Set-Content -Path $serve -Value $s2 -Encoding UTF8; Write-Host "[fix] serve.c '\0' clamp ensured" -ForegroundColor Green }
  else { Write-Host "[info] serve.c already OK or pattern not found" -ForegroundColor Yellow }
} else { Write-Host "[warn] src\serve.c not found" -ForegroundColor Yellow }

Write-Host "`nNext:" -ForegroundColor Cyan
Write-Host "  git add src\main.c src\serve.c"
Write-Host "  git commit -m 'fix(export): correct pandoc quoting; chore(serve): proper \'\0\' clamp'"
Write-Host "  git push origin main"
