/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * PURPOSE: Default LLM facade dispatcher + friendly local stub
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

/* Tiny portable strdup fallback (Windows namespaced already in common.h) */
static char* ueng_strdup(const char* s){
  if(!s) return NULL;
  size_t n = strlen(s) + 1;
  char* p = (char*)malloc(n);
  if(p) memcpy(p, s, n);
  return p;
}

/* Backend-neutral context wraps the provider-specific pointer and a tag. */
struct ueng_llm_ctx {
  int backend;     /* ueng_llm_backend tag */
  void* impl;      /* provider-specific pointer */
};

/* Parse env UENG_LLM_PROVIDER into an enum. */
static int env_backend(void){
  const char* e = getenv("UENG_LLM_PROVIDER");
  if(!e || !*e) return UENG_LLM_BACKEND_LLAMA;
  if(strcasecmp(e, "openai")==0) return UENG_LLM_BACKEND_OPENAI;
  if(strcasecmp(e, "ollama")==0) return UENG_LLM_BACKEND_OLLAMA;
  if(strcasecmp(e, "llama")==0)  return UENG_LLM_BACKEND_LLAMA;
  return UENG_LLM_BACKEND_LLAMA;
}

/* Helper to write a concise error. */
static void set_err(char* err, size_t errsz, const char* msg){
  if(err && errsz){
    snprintf(err, errsz, "%s", msg ? msg : "error");
  }
}

/* ------------------------------- Public API -------------------------------- */

ueng_llm_ctx* ueng_llm_open(const char* model_or_id, int ctx_tokens,
                            char* err, size_t errsz){
  const int be = env_backend();

  /* OPENAI */
#if defined(UAENG_ENABLE_OPENAI)
  if(be == UENG_LLM_BACKEND_OPENAI){
    extern ueng_llm_ctx* ueng_llm_open_openai(const char*, int, char*, size_t);
    ueng_llm_ctx* ctx = ueng_llm_open_openai(model_or_id, ctx_tokens, err, errsz);
    if(ctx) ctx->backend = UENG_LLM_BACKEND_OPENAI;
    return ctx;
  }
#else
  if(be == UENG_LLM_BACKEND_OPENAI){
    set_err(err, errsz, "OpenAI backend not enabled. Rebuild with UAENG_ENABLE_OPENAI=ON.");
    return NULL;
  }
#endif

  /* OLLAMA */
#if defined(UAENG_ENABLE_OLLAMA)
  if(be == UENG_LLM_BACKEND_OLLAMA){
    extern ueng_llm_ctx* ueng_llm_open_ollama(const char*, int, char*, size_t);
    ueng_llm_ctx* ctx = ueng_llm_open_ollama(model_or_id, ctx_tokens, err, errsz);
    if(ctx) ctx->backend = UENG_LLM_BACKEND_OLLAMA;
    return ctx;
  }
#else
  if(be == UENG_LLM_BACKEND_OLLAMA){
    set_err(err, errsz, "Ollama backend not enabled. Rebuild with UAENG_ENABLE_OLLAMA=ON.");
    return NULL;
  }
#endif

  /* Default: LLAMA stub (friendly message; keeps core linkable). */
  (void)model_or_id; (void)ctx_tokens;
  set_err(err, errsz,
    "Local llama backend is stubbed. "
    "Bundle llama.cpp or use UENG_LLM_PROVIDER=openai/ollama.");
  return NULL;
}

int ueng_llm_prompt(ueng_llm_ctx* ctx, const char* prompt,
                    char* out, size_t outsz){
  if(!ctx){ set_err(out, outsz, "no context"); return -1; }
  if(ctx->backend == UENG_LLM_BACKEND_OPENAI){
#if defined(UAENG_ENABLE_OPENAI)
    extern int  ueng_llm_prompt_openai(ueng_llm_ctx*, const char*, char*, size_t);
    return ueng_llm_prompt_openai(ctx, prompt, out, outsz);
#else
    return -2;
#endif
  }
  if(ctx->backend == UENG_LLM_BACKEND_OLLAMA){
#if defined(UAENG_ENABLE_OLLAMA)
    extern int  ueng_llm_prompt_ollama(ueng_llm_ctx*, const char*, char*, size_t);
    return ueng_llm_prompt_ollama(ctx, prompt, out, outsz);
#else
    return -2;
#endif
  }
  return -3; /* stub has nothing to do */
}

void ueng_llm_close(ueng_llm_ctx* ctx){
  if(!ctx) return;
  if(ctx->backend == UENG_LLM_BACKEND_OPENAI){
#if defined(UAENG_ENABLE_OPENAI)
    extern void ueng_llm_close_openai(ueng_llm_ctx*);
    ueng_llm_close_openai(ctx);
    return;
#endif
  }
  if(ctx->backend == UENG_LLM_BACKEND_OLLAMA){
#if defined(UAENG_ENABLE_OLLAMA)
    extern void ueng_llm_close_ollama(ueng_llm_ctx*);
    ueng_llm_close_ollama(ctx);
    return;
#endif
  }
  free(ctx); /* stub */
}
