# -----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# PURPOSE: In-place fix for src\main.c pandoc input quoting
# Created by: Umicom Foundation | Developer: Sammy Hegab | Date: 24-09-2025 | MIT
# -----------------------------------------------------------------------------
$ErrorActionPreference = "Stop"

$main = "src\main.c"
if (-not (Test-Path $main)) {
  Write-Host "[error] src\main.c not found" -ForegroundColor Red
  exit 1
}

$c = Get-Content $main -Raw

# 1) Fix the common broken token where \"workspace... appears alone (outside a string)
$c2 = $c -replace '(^\s*)\\\"workspace/book-draft\.md\\\"(\s*,\s*)', '$1"\"workspace/book-draft.md\""${2}'

# 2) Also fix the ""workspace... double-quote variant if present
$c2 = $c2 -replace '""workspace/book-draft\.md""','"\"workspace/book-draft.md\""'

if ($c2 -ne $c) {
  Set-Content -Path $main -Value $c2 -Encoding UTF8
  Write-Host "[fix] src\main.c adjusted" -ForegroundColor Green
} else {
  Write-Host "[info] src\main.c already correct or pattern not found" -ForegroundColor Yellow
}
