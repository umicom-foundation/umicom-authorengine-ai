
Param(
  [string]$Input = "Chapter_01_Full_Introduction_and_Development_Setup.md",
  [string]$Output = "Chapter_01_Full_Introduction_and_Development_Setup.pdf"
)
$ErrorActionPreference = "Stop"
if (-not (Get-Command pandoc -ErrorAction SilentlyContinue)) {
  Write-Error "Pandoc not found. Install pandoc and re-run."
}
pandoc $Input -o $Output --from gfm --toc --metadata title="Chapter 1 — Introduction and Development Setup"
Write-Host "Wrote $Output"
