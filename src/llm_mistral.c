/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * File: src/llm_mistral.c
 * PURPOSE: Minimal Mistral (Codestral) backend using Chat Completions API.
 *
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab + Sarah (assistant)
 * Date: 28-09-2025
 * License: MIT
 *---------------------------------------------------------------------------*
 * Implementation notes:
 * - Zero external JSON deps to keep footprint small; naive string extraction.
 * - For production, swap to yyjson/cJSON and add streaming support.
 * - Reads configuration from environment variables:
 *     MISTRAL_API_KEY        (required)
 *     UENG_MISTRAL_BASE_URL  (optional, default: https://api.mistral.ai)
 *     UENG_MISTRAL_MODEL     (optional, overrides model passed to open)
 *---------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

typedef struct ueng_llm_ctx {
  char *base_url;
  char *api_key;
  char *model;
  int   ctx_tokens;
} ueng_llm_ctx;

static char *dupstr(const char *s){
  size_t n = s ? strlen(s) : 0;
  char *p = (char*)malloc(n+1);
  if(p){ if(s) memcpy(p,s,n); p[n]=0; }
  return p;
}

static const char *getenv_def(const char *k, const char *defv){
  const char *v = getenv(k);
  return (v && *v) ? v : defv;
}

/* Write callback to collect HTTP response into dynamic buffer */
struct membuf { char *p; size_t n; size_t cap; };
static size_t mb_write(void *ptr, size_t sz, size_t nm, void *ud){
  size_t n = sz*nm; struct membuf *b = (struct membuf*)ud;
  if(!b->p){ b->cap = (n? n:1) + 4096; b->p = (char*)malloc(b->cap); b->n=0; }
  if(b->n + n + 1 > b->cap){ size_t nc = (b->cap*2) + n + 1; char *np=(char*)realloc(b->p, nc); if(!np) return 0; b->p=np; b->cap=nc; }
  memcpy(b->p + b->n, ptr, n); b->n += n; b->p[b->n] = 0; return n;
}

ueng_llm_ctx *ueng_llm_open(const char *model_or_null, int ctx_tokens, char *err, size_t errsz){
  const char *key = getenv("MISTRAL_API_KEY");
  if(!key || !*key){
    if(err && errsz) snprintf(err, errsz, "MISTRAL_API_KEY is not set");
    return NULL;
  }
  const char *base = getenv_def("UENG_MISTRAL_BASE_URL", "https://api.mistral.ai");
  const char *ovm  = getenv("UENG_MISTRAL_MODEL");
  const char *mdl  = (ovm && *ovm) ? ovm : (model_or_null && *model_or_null ? model_or_null : "mistral-small-latest");

  ueng_llm_ctx *ctx = (ueng_llm_ctx*)calloc(1, sizeof(*ctx));
  if(!ctx){ if(err && errsz) snprintf(err, errsz, "alloc failed"); return NULL; }
  ctx->api_key = dupstr(key);
  ctx->base_url = dupstr(base);
  ctx->model = dupstr(mdl);
  ctx->ctx_tokens = ctx_tokens;
  if(err && errsz) err[0]=0;
  return ctx;
}

/* naive JSON string escaper for basic quotes/backslashes */
static void json_escape(const char *in, char *out, size_t outsz){
  size_t j=0;
  for(size_t i=0; in && in[i] && j+2 < outsz; ++i){
    unsigned char c = (unsigned char)in[i];
    if(c=='\\' || c=='\"'){ if(j+2<outsz){ out[j++]='\\'; out[j++]=c; } }
    else if(c=='\n'){ if(j+2<outsz){ out[j++]='\\'; out[j++]='n'; } }
    else if(c=='\r'){ if(j+2<outsz){ out[j++]='\\'; out[j++]='r'; } }
    else { out[j++]=c; }
  }
  if(outsz) out[j<outsz?j:outsz-1]=0;
}

int ueng_llm_prompt(ueng_llm_ctx *ctx, const char *prompt, char *out, size_t outsz){
  if(!ctx || !prompt || !out || outsz==0) return 1;

  CURL *h = curl_easy_init();
  if(!h){ snprintf(out, outsz, "curl init failed"); return 2; }

  char esc[8192]; json_escape(prompt, esc, sizeof esc);

  char body[9000];
  snprintf(body, sizeof body,
    "{"
      "\"model\":\"%s\","
      "\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}],"
      "\"max_tokens\":512,"
      "\"temperature\":0.2"
    "}",
    ctx->model, esc);

  char url[512];
  snprintf(url, sizeof url, "%s/v1/chat/completions", ctx->base_url);

  struct curl_slist *hdr = NULL;
  char auth[512]; snprintf(auth, sizeof auth, "Authorization: Bearer %s", ctx->api_key);
  hdr = curl_slist_append(hdr, "Content-Type: application/json");
  hdr = curl_slist_append(hdr, auth);

  struct membuf mb = {0};
  curl_easy_setopt(h, CURLOPT_URL, url);
  curl_easy_setopt(h, CURLOPT_HTTPHEADER, hdr);
  curl_easy_setopt(h, CURLOPT_POSTFIELDS, body);
  curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, mb_write);
  curl_easy_setopt(h, CURLOPT_WRITEDATA, &mb);

  CURLcode rc = curl_easy_perform(h);
  long code = 0; curl_easy_getinfo(h, CURLINFO_RESPONSE_CODE, &code);
  curl_slist_free_all(hdr);
  curl_easy_cleanup(h);

  if(rc != CURLE_OK || code/100 != 2){
    snprintf(out, outsz, "HTTP %ld (curl rc=%d)", code, (int)rc);
    if(mb.p){ free(mb.p); }
    return 3;
  }

  /* very naive extraction: look for first "content":" ... " */
  const char *p = mb.p ? strstr(mb.p, "\"content\"") : NULL;
  if(!p){ snprintf(out, outsz, "no content in response"); free(mb.p); return 4; }
  p = strchr(p, ':'); if(!p){ snprintf(out, outsz, "bad json"); free(mb.p); return 5; }
  p++;
  while(*p && (*p==' ')) p++;
  if(*p!='\"'){ snprintf(out, outsz, "unexpected json"); free(mb.p); return 6; }
  p++; /* inside string */
  size_t j=0;
  while(*p && j+1<outsz){
    if(*p=='\\' && *(p+1)){ /* basic escapes */
      char e = *(p+1);
      if(e=='n'){ out[j++]='\n'; p+=2; continue; }
      if(e=='r'){ out[j++]='\r'; p+=2; continue; }
      if(e=='\"'){ out[j++]='\"'; p+=2; continue; }
      if(e=='\\'){ out[j++]='\\'; p+=2; continue; }
    }
    if(*p=='\"'){ break; }
    out[j++]=*p++;
  }
  out[j]=0;
  free(mb.p);
  return 0;
}

void ueng_llm_close(ueng_llm_ctx *ctx){
  if(!ctx) return;
  free(ctx->api_key);
  free(ctx->base_url);
  free(ctx->model);
  free(ctx);
}
