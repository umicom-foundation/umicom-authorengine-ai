<# ---------------------------------------------------------------------------
 Umicom AuthorEngine AI — export.ps1
 Created by: Umicom Foundation (https://umicom.foundation/)
 Author: Sammy Hegab
 Date: 2025-09-15

 PURPOSE
   Helper to export artifacts from the latest (or specified) build:
     - HTML  -> outputs\<slug>\<YYYY-MM-DD>\html\book.html
     - DOCX  -> outputs\<slug>\<YYYY-MM-DD>\docx\book.docx
     - PDF   -> outputs\<slug>\<YYYY-MM-DD>\pdf\book.pdf (headless Edge/Chrome)

 USAGE
   powershell -ExecutionPolicy Bypass -File tools\export.ps1 [-Root <path>] [-Open:$true]

   -Root:  Optional. If omitted, the script finds the newest outputs\*\*\BUILDINFO.txt.
   -Open:  Optional switch. If set, opens the HTML and PDF at the end.

 REQUIREMENTS
   - Pandoc in PATH (already installed).
   - Edge or Chrome installed (for PDF via headless print). If neither is present,
     the script will still produce HTML + DOCX, and will tell you how to install Edge
     or switch to a LaTeX toolchain later if you prefer pandoc->pdf.

 NOTES
   - This script tries to run `uaengine.exe build` first if it can find the binary
     (build-uae\uaengine.exe preferred).
--------------------------------------------------------------------------- #>

param(
  [string] $Root,
  [switch] $Open
)

$ErrorActionPreference = 'Stop'
function Info([string]$msg){ Write-Host "[export] $msg" -ForegroundColor Cyan }
function Warn([string]$msg){ Write-Host "[export] WARN: $msg" -ForegroundColor Yellow }
function Fail([string]$msg){ Write-Host "[export] ERROR: $msg" -ForegroundColor Red; exit 1 }

# 0) Try to run a fresh build if uaengine.exe is present
$uaeCandidates = @(
  ".\build-uae\uaengine.exe",
  ".\build\uaengine.exe",
  ".\build-ninja\uaengine.exe"
)
$uae = $uaeCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if ($uae) {
  Info "Running pipeline: $uae build"
  & $uae build
} else {
  Warn "uaengine.exe not found (skipping build). If needed, run: .\build-uae\uaengine.exe build"
}

# 1) Locate the target outputs root
if (-not $Root) {
  $bi = Get-ChildItem -Path ".\outputs" -Filter "BUILDINFO.txt" -Recurse -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
  if (-not $bi) {
    Fail "Could not find outputs\*\*\BUILDINFO.txt. Run a build first."
  }
  $Root = $bi.Directory.FullName
}
if (-not (Test-Path $Root)) {
  Fail "Provided -Root does not exist: $Root"
}
Info "Using build root: $Root"

# 2) Make sure Pandoc is available
try {
  $pandoc = (Get-Command pandoc).Source
  Info "Pandoc: $pandoc"
} catch {
  Fail "Pandoc not found in PATH. Install from https://pandoc.org/installing.html"
}

# 3) Decide which markdown to convert
# Prefer the version inside outputs (frozen), else workspace draft.
$BookDraft = Join-Path $Root "md\book-draft.md"
if (-not (Test-Path $BookDraft)) {
  $BookDraft = ".\workspace\book-draft.md"
}
if (-not (Test-Path $BookDraft)) {
  Fail "book-draft.md not found. Run a build first."
}

# 4) Parse BUILDINFO for Title/Author/Slug/Date (for metadata + messages)
$BuildInfoPath = Join-Path $Root "BUILDINFO.txt"
$Title  = "Untitled"
$Author = "Unknown"
$Slug   = "untitled"
$Date   = ""
if (Test-Path $BuildInfoPath) {
  $lines = Get-Content $BuildInfoPath
  foreach ($L in $lines) {
    if ($L -match '^Title:\s*(.*)$')  { $Title  = $Matches[1].Trim() }
    elseif ($L -match '^Author:\s*(.*)$'){ $Author = $Matches[1].Trim() }
    elseif ($L -match '^Slug:\s*(.*)$')  { $Slug   = $Matches[1].Trim() }
    elseif ($L -match '^Date:\s*(.*)$')  { $Date   = $Matches[1].Trim() }
  }
}
Info "Title=$Title | Author=$Author | Slug=$Slug | Date=$Date"

# 5) Ensure output subfolders exist
$HtmlDir = Join-Path $Root "html"
$PdfDir  = Join-Path $Root "pdf"
$DocxDir = Join-Path $Root "docx"
New-Item -ItemType Directory -Force -Path $HtmlDir,$PdfDir,$DocxDir | Out-Null

# 6) Write a minimal CSS for nicer HTML typography
$CssPath = Join-Path $HtmlDir "uae-style.css"
@"
body{font-family:system-ui,-apple-system,"Segoe UI",Roboto,Ubuntu,sans-serif;max-width:900px;margin:2rem auto;line-height:1.6}
h1,h2,h3{line-height:1.25}
code,pre{background:#f6f8fa;padding:.2rem .4rem;border-radius:.4rem}
a{color:#0ea5e9;text-decoration:none} a:hover{text-decoration:underline}
blockquote{border-left:4px solid #ddd;margin:1rem 0;padding:.5rem 1rem;color:#555}
hr{border:none;border-top:1px solid #e5e7eb;margin:2rem 0}
"@ | Set-Content -Encoding UTF8 $CssPath

# 7) Export HTML
$BookHtml = Join-Path $HtmlDir "book.html"
Info "Exporting HTML -> $BookHtml"
& $pandoc `
  --from markdown `
  --to html5 `
  --standalone `
  --embed-resources `
  --toc `
  --css "$CssPath" `
  --metadata title="$Title" `
  --metadata author="$Author" `
  --output "$BookHtml" `
  "$BookDraft"

# 8) Export DOCX (no LaTeX needed)
$BookDocx = Join-Path $DocxDir "book.docx"
Info "Exporting DOCX -> $BookDocx"
& $pandoc `
  --from markdown `
  --to docx `
  --metadata title="$Title" `
  --metadata author="$Author" `
  --output "$BookDocx" `
  "$BookDraft"

# 9) Export PDF via Edge/Chrome headless (uses the HTML we just generated)
$BookPdf = Join-Path $PdfDir "book.pdf"
$edgePaths = @(
  "$env:ProgramFiles (x86)\Microsoft\Edge\Application\msedge.exe",
  "$env:ProgramFiles\Microsoft\Edge\Application\msedge.exe"
)
$chromePaths = @(
  "$env:ProgramFiles\Google\Chrome\Application\chrome.exe",
  "$env:ProgramFiles (x86)\Google\Chrome\Application\chrome.exe"
)
$browser = ($edgePaths + $chromePaths) | Where-Object { Test-Path $_ } | Select-Object -First 1

if ($browser) {
  Info "Exporting PDF (headless) via $([System.IO.Path]::GetFileName($browser)) -> $BookPdf"
  $fileUrl = "file:///" + ((Resolve-Path $BookHtml).Path -replace '\\','/')
  & $browser --headless=new --disable-gpu "--print-to-pdf=$BookPdf" --no-margins "$fileUrl" | Out-Null
  if (-not (Test-Path $BookPdf)) {
    Warn "Browser did not produce $BookPdf. You can still open HTML/DOCX. For PDF via pandoc, install a LaTeX engine later."
  }
} else {
  Warn "Edge/Chrome not found. Skipping PDF. Install Edge or Chrome, or tell me and I’ll wire a LaTeX-based PDF path."
}

# 10) Optionally open results
if ($Open) {
  if (Test-Path $BookHtml) { Start-Process "$BookHtml" }
  if (Test-Path $BookPdf)  { Start-Process "$BookPdf" }
  if (Test-Path $BookDocx) { Start-Process "$BookDocx" }
}

Info "Done. Outputs:"
Info "  HTML : $BookHtml"
Info "  DOCX : $BookDocx"
if (Test-Path $BookPdf) { Info "  PDF  : $BookPdf" } else { Warn "  PDF  : (not generated)" }
