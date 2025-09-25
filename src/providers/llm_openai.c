/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * PURPOSE: OpenAI provider (stubbed) — builds opt-in with UAENG_ENABLE_OPENAI
 *
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab
 * Date: 24-09-2025
 * License: MIT
 *----------------------------------------------------------------------------*/
#include "ueng/llm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Private concrete ctx for OpenAI. Here we only store placeholders so we
 * can build without linking any HTTP/JSON libs. */
struct ueng_llm_ctx {
  int backend;
  char* model;
};

ueng_llm_ctx* ueng_llm_open_openai(const char* model, int ctx_tokens,
                                   char* err, size_t errsz){
  (void)ctx_tokens;
  if(!model || !*model){
    if(err) snprintf(err, errsz, "OpenAI: model id is empty");
    return NULL;
  }
  ueng_llm_ctx* c = (ueng_llm_ctx*)calloc(1, sizeof(*c));
  if(!c){ if(err) snprintf(err, errsz, "OpenAI: out of memory"); return NULL; }
  c->backend = UENG_LLM_BACKEND_OPENAI;
  c->model = _strdup ? _strdup(model) : strdup(model);
  return c;
}

int ueng_llm_prompt_openai(ueng_llm_ctx* ctx, const char* prompt,
                           char* out, size_t outsz){
  if(!ctx || !prompt){ return -1; }
  if(out && outsz){
    snprintf(out, outsz,
      "[OpenAI stub] Would send prompt to model '%s'. "
      "Rebuild with real HTTP client to enable.", ctx->model ? ctx->model : "gpt-4o");
  }
  return 0;
}

void ueng_llm_close_openai(ueng_llm_ctx* ctx){
  if(!ctx) return;
  if(ctx->model) free(ctx->model);
  free(ctx);
}
