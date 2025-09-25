/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * PURPOSE: LLM public facade and provider abstraction (comments preserved & expanded)
 *
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab
 * Date: 24-09-2025
 * License: MIT
 *---------------------------------------------------------------------------*/
#ifndef UENG_LLM_H
#define UENG_LLM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /*------------------------------ Overview -------------------------------------
    Tiny, provider‑agnostic LLM facade used by uaengine.

    Why this shape?
     - Keep CLI/commands independent of any particular LLM runtime.
     - Allow multiple backends (local llama.cpp, OpenAI API, Ollama) to plug in.
     - Avoid imposing heavy dependencies on the core build by default.

    How selection works
     - At runtime, uaengine checks the environment variable UENG_LLM_PROVIDER
       (llama | openai | ollama). Unknown values fall back to "llama".
     - If the chosen backend is not compiled in, open returns NULL with a
       human‑friendly error explaining how to enable it.
   -----------------------------------------------------------------------------*/

  /* Opaque LLM session context. Concrete shape is backend‑specific. */
  typedef struct ueng_llm_ctx ueng_llm_ctx;

  /* Recognized backends (kept small & stable). */
  typedef enum ueng_llm_backend
  {
    UENG_LLM_BACKEND_LLAMA = 0,  /* local llama.cpp (GGUF)            */
    UENG_LLM_BACKEND_OPENAI = 1, /* remote OpenAI API (opt‑in)        */
    UENG_LLM_BACKEND_OLLAMA = 2  /* local Ollama HTTP API (opt‑in)    */
  } ueng_llm_backend;

  /*-----------------------------------------------------------------------------
   * ueng_llm_open: Create an LLM session.
   *
   *  model_or_id : GGUF path (llama), or model id/name (openai/ollama).
   *  ctx_tokens  : desired context length (backends may clamp/ignore).
   *  err         : output buffer for a human‑friendly error on failure.
   *  errsz       : size of 'err' in bytes.
   *
   * RETURNS: non‑NULL context on success, NULL on error (with 'err' filled).
   *
   * This function ALWAYS exists. When the requested backend is unavailable,
   * it returns NULL with a clear message; this keeps the main binary linkable
   * everywhere while remaining friendly to users.
   *---------------------------------------------------------------------------*/
  ueng_llm_ctx *ueng_llm_open(const char *model_or_id, int ctx_tokens, char *err, size_t errsz);

  /*-----------------------------------------------------------------------------
   * ueng_llm_prompt: Run a simple prompt and collect a short completion.
   *
   *  ctx    : context obtained from ueng_llm_open.
   *  prompt : UTF‑8 input text.
   *  out    : output buffer for the generated text.
   *  outsz  : size of 'out' in bytes (including NUL terminator).
   *
   * RETURNS: 0 on success; non‑zero on failure.
   *
   * This is intentionally tiny; we can extend it later to stream tokens or
   * expose advanced sampling. For now it lets us verify end‑to‑end wiring.
   *---------------------------------------------------------------------------*/
  int ueng_llm_prompt(ueng_llm_ctx *ctx, const char *prompt, char *out, size_t outsz);

  /*-----------------------------------------------------------------------------
   * ueng_llm_close: Destroy and free the context (safe to call with NULL).
   *---------------------------------------------------------------------------*/
  void ueng_llm_close(ueng_llm_ctx *ctx);

/*-------------------- Optional provider‑specific entrypoints ------------------
  These exist only when a provider is compiled with UAENG_ENABLE_xxx=ON.
  No production code should call them directly; ueng_llm_open() dispatches.
 -----------------------------------------------------------------------------*/
#ifdef UAENG_ENABLE_OPENAI
  ueng_llm_ctx *ueng_llm_open_openai(const char *model, int ctx_tokens, char *err, size_t errsz);
  int ueng_llm_prompt_openai(ueng_llm_ctx *ctx, const char *prompt, char *out, size_t outsz);
  void ueng_llm_close_openai(ueng_llm_ctx *ctx);
#endif

#ifdef UAENG_ENABLE_OLLAMA
  ueng_llm_ctx *ueng_llm_open_ollama(const char *model, int ctx_tokens, char *err, size_t errsz);
  int ueng_llm_prompt_ollama(ueng_llm_ctx *ctx, const char *prompt, char *out, size_t outsz);
  void ueng_llm_close_ollama(ueng_llm_ctx *ctx);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
#endif /* UENG_LLM_H */
