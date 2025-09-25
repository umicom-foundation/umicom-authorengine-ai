#!/usr/bin/env bash
set -euo pipefail
main="src/main.c"
[ -f "$main" ] || { echo "[error] not found: $main"; exit 1; }

if grep -q 'static int cmd_llm_selftest' "$main"; then
  echo "[info] llm-selftest already present; nothing to do."
  exit 0
fi

snippet='
/*--------------------------------------------------------------------------
 * Inserted by scripts/fix/add-llm-selftest.*
 * PURPOSE: Add '"'"'llm-selftest'"'"' command to exercise the embedded LLM wrapper.
 *---------------------------------------------------------------------------*/
#include "ueng/llm.h"

static int cmd_llm_selftest(int argc, char** argv) {
  const char* model = NULL;
  if (argc >= 3) model = argv[2];
  if (!model || !*model) {
    model = getenv("UENG_LLM_MODEL");
  }
  if (!model || !*model) {
    fprintf(stderr, "[llm-selftest] ERROR: no model path given and UENG_LLM_MODEL not set.\n");
    return 2;
  }

  char err[256] = {0};
  ueng_llm_ctx* L = ueng_llm_open(model, 4096, err, sizeof(err));
  if (!L) {
    fprintf(stderr, "[llm-selftest] open failed: %s\n", err[0] ? err : "(unknown)");
    return 3;
  }
  char out[2048] = {0};
  int rc = ueng_llm_prompt(L, "Say hello from AuthorEngine.", out, sizeof(out));
  if (rc == 0) {
    printf("%s\n", out);
  } else {
    fprintf(stderr, "[llm-selftest] prompt failed (rc=%d)\n", rc);
  }
  ueng_llm_close(L);
  return rc;
}
'

if grep -qE $'^\s*int\s+main\s*\(' "$main"; then
  awk -v SNIP="$snippet" '
    BEGIN{printed=0}
    /int[ \t]+main[ \t]*\(/ && !printed { print SNIP; printed=1 }
    { print }
  ' "$main" > "$main.tmp" && mv "$main.tmp" "$main"
else
  printf "%s\n" "$snippet" >> "$main"
fi

if grep -q 'Commands:' "$main"; then
  awk '
    BEGIN{added=0}
    { print }
    /Commands:/ && !added { print "  llm-selftest [model.gguf]  LLM embed smoke test (optional)"; added=1 }
  ' "$main" > "$main.tmp" && mv "$main.tmp" "$main"
fi

if grep -q 'const char *cmd = argv\[1\];' "$main"; then
  sed -i.bak 's/const char \*cmd = argv\[1\];/const char *cmd = argv[1];\
  if (strcmp(cmd, "llm-selftest") == 0) return cmd_llm_selftest(argc, argv);/g' "$main"
elif grep -qE '^\s*if\s*\(\s*strcmp\s*\(\s*cmd\s*,\s*"init"\s*\)\s*==\s*0\s*\)\s*\{' "$main"; then
  sed -i.bak $'0,/^\s*if\s*\(\s*strcmp\s*\(\s*cmd\s*,\s*"init"\s*\)\s*==\s*0\s*\)\s*\{/s//  if (strcmp(cmd, "llm-selftest") == 0) return cmd_llm_selftest(argc, argv);\n&/' "$main"
else
  echo "[warn] Could not find dispatch anchor; please add manually near the command ladder."
fi

echo "[ok] 'llm-selftest' inserted. Rebuild to test."
