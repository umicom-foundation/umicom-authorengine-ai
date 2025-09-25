/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * File: include/ueng/llm.h
 * PURPOSE: Public C API for local-LLM embedding (llama.cpp backend).
 *
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab
 * Date: 24-09-2025
 * License: MIT
 *---------------------------------------------------------------------------*/
#ifndef UENG_LLM_H
#define UENG_LLM_H

#include <stddef.h> /* size_t */

#ifdef __cplusplus
extern "C"
{
#endif

  /* Forward-declared opaque context for an embedded LLM session. */
  typedef struct ueng_llm_ctx ueng_llm_ctx;

  /* ueng_llm_open: Create an in-process LLM context.
   *
   * model_path : filesystem path to a GGUF model (only used when the real
   *              llama.cpp backend is compiled in).
   * ctx_tokens : desired context length (e.g., 4096).
   * err        : output buffer for a human-friendly error on failure.
   * errsz      : size of 'err' in bytes.
   *
   * RETURNS: non-NULL context on success, NULL on error (with 'err' filled).
   *
   * This function ALWAYS exists. When the embedded backend is not available,
   * it returns NULL with an explanatory error; this keeps the main binary
   * linkable and friendly for environments that do not bundle llama.cpp.
   */
  ueng_llm_ctx *ueng_llm_open(const char *model_path, int ctx_tokens, char *err, size_t errsz);

  /* ueng_llm_prompt: Run a simple prompt and collect a short completion.
   *
   * ctx    : context obtained from ueng_llm_open.
   * prompt : UTF-8 input text.
   * out    : output buffer for the generated text.
   * outsz  : size of 'out' in bytes (including NUL terminator).
   *
   * RETURNS: 0 on success; non-zero on failure.
   *
   * This is intentionally tiny; we can extend it later to stream tokens or
   * expose advanced sampling. For now it lets us verify end-to-end wiring.
   */
  int ueng_llm_prompt(ueng_llm_ctx *ctx, const char *prompt, char *out, size_t outsz);

  /* ueng_llm_close: Destroy and free the context (safe to call with NULL). */
  void ueng_llm_close(ueng_llm_ctx *ctx);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* UENG_LLM_H */
