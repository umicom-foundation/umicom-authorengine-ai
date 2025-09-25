/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * PURPOSE: LLM public facade + provider abstraction
 *
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab
 * Date: 24-09-2025
 * License: MIT
 *----------------------------------------------------------------------------*/
#ifndef UENG_LLM_H
#define UENG_LLM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------- Overview ----------------------------------
 * This header defines a tiny, provider-agnostic LLM facade used by uaengine.
 *
 * Why this shape?
 * - Keep the CLI and commands independent of any particular LLM runtime.
 * - Allow multiple backends (local llama.cpp, OpenAI API, Ollama) to plug in.
 * - Avoid imposing heavy dependencies on the core build by default.
 *
 * How selection works
 * - By default, uaengine compiles a minimal "llama" stub that returns a
 *   friendly error explaining how to enable a provider.
 * - At runtime, uaengine reads UENG_LLM_PROVIDER to decide which backend to
 *   use. (e.g., UENG_LLM_PROVIDER=openai or ollama or llama)
 * - If that provider was not compiled in, the open call returns NULL with an
 *   explanatory error; this keeps binaries linkable everywhere.
 * --------------------------------------------------------------------------*/

/* Opaque LLM session context. The concrete shape is backend-specific. */
typedef struct ueng_llm_ctx ueng_llm_ctx;

/* Recognized backends (environment selects the entry). */
typedef enum ueng_llm_backend {
  UENG_LLM_BACKEND_LLAMA = 0,  /* local llama.cpp (GGUF) */
  UENG_LLM_BACKEND_OPENAI = 1, /* remote OpenAI API       */
  UENG_LLM_BACKEND_OLLAMA = 2  /* local Ollama HTTP API   */
} ueng_llm_backend;

/* ueng_llm_open: Create an LLM session.
 *  model_or_id : GGUF path (llama), model id/name (openai/ollama)
 *  ctx_tokens  : hint for context length (backends may ignore)
 *  err/errsz   : human-friendly error buffer on failure
 *
 * Returns non-NULL on success, NULL on failure (with 'err' filled).
 * This always exists; if a selected backend isn't compiled, we still return
 * NULL and explain how to enable it.
 */
ueng_llm_ctx* ueng_llm_open(const char* model_or_id, int ctx_tokens,
                            char* err, size_t errsz);

/* ueng_llm_prompt: Ask for a short completion.
 * Returns 0 on success; non-zero on failure.
 */
int ueng_llm_prompt(ueng_llm_ctx* ctx, const char* prompt,
                    char* out, size_t outsz);

/* ueng_llm_close: Destroy and free the context (safe to pass NULL). */
void ueng_llm_close(ueng_llm_ctx* ctx);

/* -------------------- Optional provider-specific entrypoints ---------------
 * These exist only when the provider is compiled with UAENG_ENABLE_xxx=ON.
 * No code should call them directly; ueng_llm_open() dispatches internally.
 */
#ifdef UAENG_ENABLE_OPENAI
ueng_llm_ctx* ueng_llm_open_openai(const char* model, int ctx_tokens,
                                   char* err, size_t errsz);
int  ueng_llm_prompt_openai(ueng_llm_ctx* ctx, const char* prompt,
                            char* out, size_t outsz);
void ueng_llm_close_openai(ueng_llm_ctx* ctx);
#endif

#ifdef UAENG_ENABLE_OLLAMA
ueng_llm_ctx* ueng_llm_open_ollama(const char* model, int ctx_tokens,
                                   char* err, size_t errsz);
int  ueng_llm_prompt_ollama(ueng_llm_ctx* ctx, const char* prompt,
                            char* out, size_t outsz);
void ueng_llm_close_ollama(ueng_llm_ctx* ctx);
#endif

#ifdef __cplusplus
} /* extern C */
#endif
#endif /* UENG_LLM_H */
