/*--------------------------------------------------------------------------
 * Inserted by scripts/fix/add-llm-selftest.*
 * PURPOSE: Add 'llm-selftest' command to exercise the embedded LLM wrapper.
 *---------------------------------------------------------------------------*/
#include "ueng/llm.h"

static int cmd_llm_selftest(int argc, char **argv)
{
  const char *model = NULL;
  if (argc >= 3)
    model = argv[2];
  if (!model || !*model)
  {
    model = getenv("UENG_LLM_MODEL");
  }
  if (!model || !*model)
  {
    fprintf(stderr, "[llm-selftest] ERROR: no model path given and UENG_LLM_MODEL not set.\n");
    return 2;
  }

  char err[256] = {0};
  ueng_llm_ctx *L = ueng_llm_open(model, 4096, err, sizeof(err));
  if (!L)
  {
    fprintf(stderr, "[llm-selftest] open failed: %s\n", err[0] ? err : "(unknown)");
    return 3;
  }
  char out[2048] = {0};
  int rc = ueng_llm_prompt(L, "Say hello from AuthorEngine.", out, sizeof(out));
  if (rc == 0)
  {
    printf("%s\n", out);
  }
  else
  {
    fprintf(stderr, "[llm-selftest] prompt failed (rc=%d)\n", rc);
  }
  ueng_llm_close(L);
  return rc;
}
