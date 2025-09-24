#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# PURPOSE: Fix 'stray \ in program' by ensuring the input path is inside a string
# Created by: Umicom Foundation | Developer: Sammy Hegab | Date: 24-09-2025 | MIT
# -----------------------------------------------------------------------------
set -euo pipefail

f="src/main.c"
[ -f "$f" ] || { echo "src/main.c not found"; exit 1; }

# Replace   \"workspace/book-draft.md\"",   with   "\"workspace/book-draft.md\"", (inside string)
perl -0777 -pe 's/(^\s*)\\\"workspace\/book-draft\.md\\\"\",\s*/${1}"\\\"workspace\/book-draft.md\\\"",\n/m' -i "$f"
# Variant: comma on next line
perl -0777 -pe 's/(^\s*)\\\"workspace\/book-draft\.md\\\"\s*,\s*$/${1}"\\\"workspace\/book-draft.md\\\"",/m' -i "$f"
# Variant: no comma (just wrap in quotes)
perl -0777 -pe 's/(^\s*)\\\"workspace\/book-draft\.md\\\"\s*$/${1}"\\\"workspace\/book-draft.md\\\""/m' -i "$f"

echo "[fix] $f corrected (book-draft path now inside string)"
