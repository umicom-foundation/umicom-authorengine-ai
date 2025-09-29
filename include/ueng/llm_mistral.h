/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * File: include/ueng/llm_mistral.h
 * PURPOSE: Declaration for the Mistral (Codestral) backend context.
 *---------------------------------------------------------------------------*/
#ifndef UENG_LLM_MISTRAL_H
#define UENG_LLM_MISTRAL_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ueng_llm_ctx ueng_llm_ctx;

/* Factory functions are declared in llm.h:
   ueng_llm_ctx *ueng_llm_open(const char *model, int ctx_tokens, char *err, size_t errsz);
   int ueng_llm_prompt(ueng_llm_ctx *ctx, const char *prompt, char *out, size_t outsz);
   void ueng_llm_close(ueng_llm_ctx *ctx);
*/

#ifdef __cplusplus
}
#endif

#endif /* UENG_LLM_MISTRAL_H */
