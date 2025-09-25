/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * PURPOSE: Ollama provider (stubbed) — builds opt-in with UAENG_ENABLE_OLLAMA
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

struct ueng_llm_ctx {
  int backend;
  char* model;
};

ueng_llm_ctx* ueng_llm_open_ollama(const char* model, int ctx_tokens,
                                   char* err, size_t errsz){
  (void)ctx_tokens;
  if(!model || !*model){
    if(err) snprintf(err, errsz, "Ollama: model name is empty");
    return NULL;
  }
  ueng_llm_ctx* c = (ueng_llm_ctx*)calloc(1, sizeof(*c));
  if(!c){ if(err) snprintf(err, errsz, "Ollama: out of memory"); return NULL; }
  c->backend = UENG_LLM_BACKEND_OLLAMA;
  c->model = _strdup ? _strdup(model) : strdup(model);
  return c;
}

int ueng_llm_prompt_ollama(ueng_llm_ctx* ctx, const char* prompt,
                           char* out, size_t outsz){
  if(!ctx || !prompt){ return -1; }
  if(out && outsz){
    snprintf(out, outsz,
      "[Ollama stub] Would POST to local Ollama for model '%s'.",
      ctx->model ? ctx->model : "llama3");
  }
  return 0;
}

void ueng_llm_close_ollama(ueng_llm_ctx* ctx){
  if(!ctx) return;
  if(ctx->model) free(ctx->model);
  free(ctx);
}
