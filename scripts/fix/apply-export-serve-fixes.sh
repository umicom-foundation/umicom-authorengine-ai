#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# PURPOSE: In-place fixes for main.c (pandoc quoting) and serve.c ('\0' clamp)
# Created by: Umicom Foundation | Developer: Sammy Hegab | Date: 24-09-2025 | MIT
# -----------------------------------------------------------------------------
set -euo pipefail

if [ -f src/main.c ]; then
  cp src/main.c src/main.c.bak
  sed -i '' -e 's/""workspace\/book-draft\.md""/"\\"workspace\/book-draft.md\\""/g' src/main.c 2>/dev/null || \
  sed -i    -e 's/""workspace\/book-draft\.md""/"\\"workspace\/book-draft.md\\""/g' src/main.c
  perl -0777 -pe 's/(^\s*)\\\"workspace\/book-draft\.md\\\"(\s*,\s*)/${1}"\\\"workspace\/book-draft.md\\\""${2}/m' -i src/main.c
  echo "[fix] main.c quoting adjusted"
else
  echo "[warn] src/main.c not found"
fi

if [ -f src/serve.c ]; then
  cp src/serve.c src/serve.c.bak
  sed -i '' -e "s/'<U+0000>'/'\\\0'/g" src/serve.c 2>/dev/null || \
  sed -i    -e "s/'<U+0000>'/'\\\0'/g" src/serve.c
  echo "[fix] serve.c '\0' clamp ensured"
else
  echo "[warn] src/serve.c not found"
fi

echo
echo "Next:"
echo "  git add src/main.c src/serve.c"
echo "  git commit -m \"fix(export): correct pandoc quoting; chore(serve): proper '\\0' clamp\""
echo "  git push origin main"
