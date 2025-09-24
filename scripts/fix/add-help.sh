#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# File: scripts/fix/add-help.sh
# PURPOSE: Non-destructive insertion of --help/-h handler into src/main.c
#
# Created by: Umicom Foundation (https://umicom.foundation/)
# Author: Sammy Hegab
# Date: 24-09-2025
# License: MIT
# -----------------------------------------------------------------------------
set -euo pipefail
file="src/main.c"
[[ -f "$file" ]] || { echo "Not found: $file (run from repo root)"; exit 1; }

if grep -qE '\bstrcmp\s*\(\s*argv\[1\]\s*,\s*"--help"\s*\)' "$file"; then
  echo "[info] --help handler already present; no changes made."
  exit 0
fi

# Use awk to insert after the closing brace of the --version block
awk '
  BEGIN { inserted=0 }
  {
    print $0
    if (!inserted && $0 ~ /\}\s*$/ && prev ~ /strcmp\(argv\[1\], "--version"\)/) {
      print "  /* -----------------------------------------------------------------------"
      print "   * Accept '"'"'--help'"'"' / '"'"'-h'"'"' explicitly and exit 0 (so CI steps never fail"
      print "   * when checking the help text). This is additive and does not change any"
      print "   * existing behavior. '"'"'uaengine help'"'"' remains supported below."
      print "   * Created by: Umicom Foundation (https://umicom.foundation/)"
      print "   * Author: Sammy Hegab"
      print "   * Date: 24-09-2025"
      print "   * --------------------------------------------------------------------- */"
      print "  if ((strcmp(argv[1], \"--help\") == 0) || (strcmp(argv[1], \"-h\") == 0)) {"
      print "    usage();"
      print "    return 0;"
      print "  }"
      inserted=1
    }
    prev=$0
  }
' "$file" > "$file.tmp"

mv "$file.tmp" "$file"
echo "[ok] Inserted --help/-h handler."