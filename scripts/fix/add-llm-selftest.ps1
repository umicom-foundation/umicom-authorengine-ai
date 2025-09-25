<#-----------------------------------------------------------------------------
 Umicom AuthorEngine AI (uaengine)
 File: scripts/fix/add-llm-selftest.ps1
 PURPOSE: Non-destructive insertion of 'llm-selftest' command into src\main.c

 Created by: Umicom Foundation (https://umicom.foundation/)
 Author: Sammy Hegab
 Date: 24-09-2025
 License: MIT
-----------------------------------------------------------------------------#>
$ErrorActionPreference = "Stop"
$main = "src\main.c"
if (-not (Test-Path $main)) { Write-Error "Not found: $main"; exit 1 }

$content = Get-Content $main -Raw

if ($content -match "static int cmd_llm_selftest") {
  Write-Host "[info] llm-selftest already present; nothing to do."
  exit 0
}

# 1) Insert handler function before 'int main(' if possible; else append.
$snippet = @'
/*--------------------------------------------------------------------------
 * Inserted by scripts/fix/add-llm-selftest.*
 * PURPOSE: Add 'llm-selftest' command to exercise the embedded LLM wrapper.
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
'@

if ($content -match "(?s)\bint\s+main\s*\(") {
  $content = $content -replace "(?s)(\bint\s+main\s*\()", "`n$snippet`n`$1"
} else {
  $content = $content + "`n$snippet`n"
}

# 2) Add usage line under the help/usage 'Commands:' section
$content = $content -replace "(?m)(^\s*Commands:\s*\r?\n)", "`$1  llm-selftest [model.gguf]  LLM embed smoke test (optional)`r`n"

# 3) Dispatch: after 'const char *cmd = argv[1];' inject a branch for llm-selftest.
if ($content -match "const\s+char\s*\*\s*cmd\s*=\s*argv\[1\]\s*;") {
  $content = $content -replace "const\s+char\s*\*\s*cmd\s*=\s*argv\[1\]\s*;", 'const char *cmd = argv[1];
  if (strcmp(cmd, "llm-selftest") == 0) return cmd_llm_selftest(argc, argv);'
} elseif ($content -match "(?m)^\s*if\s*\(\s*strcmp\s*\(\s*cmd\s*,\s*""init""\s*\)\s*==\s*0\s*\)\s*\{") {
  $content = $content -replace "(?m)^\s*if\s*\(\s*strcmp\s*\(\s*cmd\s*,\s*""init""\s*\)\s*==\s*0\s*\)\s*\{", "  if (strcmp(cmd, ""llm-selftest"") == 0) return cmd_llm_selftest(argc, argv);`r`n$0"
} else {
  Write-Warning "[warn] Could not find command dispatch anchor; please add manually: if (strcmp(cmd, \"llm-selftest\")==0) return cmd_llm_selftest(argc, argv);"
}

Set-Content -Path $main -Value $content -NoNewline
Write-Host "[ok] 'llm-selftest' inserted. Please rebuild."
