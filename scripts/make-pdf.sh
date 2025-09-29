#!/usr/bin/env bash
set -euo pipefail
in="Chapter_01_Full_Introduction_and_Development_Setup.md"
out="Chapter_01_Full_Introduction_and_Development_Setup.pdf"
if ! command -v pandoc >/dev/null 2>&1; then
  echo "Pandoc not found. Install pandoc and re-run." >&2
  exit 1
fi
# Use default template; add --toc or --pdf-engine if you have LaTeX
pandoc "$in" -o "$out" --from gfm --toc --metadata title="Chapter 1 — Introduction and Development Setup"
echo "Wrote $out"
