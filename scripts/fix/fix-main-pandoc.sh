#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# PURPOSE: In-place fix for src/main.c pandoc input quoting
# Created by: Umicom Foundation | Developer: Sammy Hegab | Date: 24-09-2025 | MIT
# -----------------------------------------------------------------------------
set -euo pipefail

f="src/main.c"
[ -f "$f" ] || { echo "[error] $f not found"; exit 1; }

# Fix the token when it appears alone
perl -0777 -pe 's/(^\s*)\\\"workspace\/book-draft\.md\\\"(\s*,\s*)/${1}"\\\"workspace\/book-draft.md\\\""${2}/m' -i "$f"

# Fix the ""workspace... variant
sed -i.bak 's/""workspace\/book-draft\.md""/"\\"workspace\/book-draft.md\\""/g' "$f" || true

echo "[fix] $f adjusted"
