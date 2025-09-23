/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * File: include/ueng/serve.h
 * Purpose: Tiny static HTTP server API
 *---------------------------------------------------------------------------*/
#ifndef UENG_SERVE_H
#define UENG_SERVE_H
#ifdef __cplusplus
extern "C" {
#endif

int serve_run(const char* root, const char* host, int port);

#ifdef __cplusplus
}
#endif
#endif /* UENG_SERVE_H */
