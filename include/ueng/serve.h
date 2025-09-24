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

/* Serve files under 'root' (must contain index.html). host: "127.0.0.1", port: 8080.
   Blocks in the accept loop; Ctrl+C to stop (Windows handler installed). */
int serve_run(const char* root, const char* host, int port);

#ifdef __cplusplus
}
#endif
#endif /* UENG_SERVE_H */
