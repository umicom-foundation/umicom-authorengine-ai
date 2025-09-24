# -----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# PURPOSE: Fix 'stray \ in program' by ensuring the input path is inside a string
# Created by: Umicom Foundation | Developer: Sammy Hegab | Date: 24-09-2025 | MIT
# -----------------------------------------------------------------------------
$ErrorActionPreference = "Stop"

$main = "src\main.c"
if (!(Test-Path $main)) { Write-Error "src\main.c not found"; exit 1 }

$c = Get-Content $main -Raw

# Replace any raw token   \"workspace/book-draft.md\"",   with   "\"workspace/book-draft.md\"",
$c1 = $c -replace '(^\s*)\\\"workspace/book-draft\.md\\\"\",\s*', '$1"\"workspace/book-draft.md\"",\n'
# Also handle a variant with trailing comma on next line
$c1 = $c1 -replace '(^\s*)\\\"workspace/book-draft\.md\\\"\s*,\s*$', '$1"\"workspace/book-draft.md\"",'

# And handle the no-comma case (very rare) to just wrap in quotes
$c1 = $c1 -replace '(^\s*)\\\"workspace/book-draft\.md\\\"\s*$', '$1"\"workspace/book-draft.md\""'

if ($c1 -ne $c) {
  Set-Content -Path $main -Value $c1 -Encoding UTF8
  Write-Host "[fix] src\main.c corrected (book-draft path now inside string)" -ForegroundColor Green
} else {
  Write-Host "[info] src\main.c unchanged (pattern not found or already correct)" -ForegroundColor Yellow
}

Write-Host "Rebuild after committing:"
Write-Host "  git add src\main.c"
Write-Host "  git commit -m 'fix(export): keep book-draft path inside format string'"
Write-Host "  git push origin main"
