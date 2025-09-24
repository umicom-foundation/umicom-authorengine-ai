/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * File: src/serve.c
 * PURPOSE: Tiny cross-platform static file HTTP server (hardened URL handling)
 *
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab + contributors
 * Date: 15-09-2025
 * License: MIT
 *---------------------------------------------------------------------------*/
#include "ueng/serve.h"
#include "ueng/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "Ws2_32.lib")
  typedef SOCKET ueng_socket_t;
  #define closesock closesocket
  /* Keep Ctrl+C from killing the process mid-send on Windows */
  static BOOL WINAPI ctrl_handler(DWORD type){ (void)type; return TRUE; }
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <signal.h>
  typedef int ueng_socket_t;
  #define closesock close
#endif

/*------------------------------ helpers ------------------------------------*/
/* Hex digit -> value for %XX decoding */
static int hexval(unsigned char c){
  if (c>='0' && c<='9') return c - '0';
  if (c>='a' && c<='f') return c - 'a' + 10;
  if (c>='A' && c<='F') return c - 'A' + 10;
  return -1;
}

/* Strict in-place URL decoder (handles %XX, + -> space not applied here).
   Returns 0 on success, -1 on bad encoding. */
static int url_decode(const char* in, char* out, size_t outsz){
  size_t j = 0;
  for (size_t i=0; in[i] != '\0'; ++i){
    unsigned char c = (unsigned char)in[i];
    if (c == '%'){
      int h1 = hexval((unsigned char)in[i+1]);
      int h2 = hexval((unsigned char)in[i+2]);
      if (h1<0 || h2<0) return -1;
      unsigned char v = (unsigned char)((h1<<4) | h2);
      i += 2;
      if (j+1 >= outsz) return -1;
      out[j++] = (char)v;
    }else{
      if (j+1 >= outsz) return -1;
      out[j++] = (char)c;
    }
  }
  if (j >= outsz) return -1;
  out[j] = '\0';
  return 0;
}

/* Basic traversal guard after decoding; rejects any ".." segments. */
static int path_is_traversal(const char* s){
  if(!s) return 1;
  if (strstr(s, "..") != NULL) return 1;
  if (strchr(s, '\0') == NULL) return 1; /* paranoia */
  return 0;
}

/* case-insensitive ASCII compare */
static int strequal_ci(const char* a, const char* b){
  unsigned char ca, cb;
  while(*a && *b){
    ca = (unsigned char)*a++; cb = (unsigned char)*b++;
    if(ca>='A'&&ca<='Z') ca = (unsigned char)(ca - 'A' + 'a');
    if(cb>='A'&&cb<='Z') cb = (unsigned char)(cb - 'A' + 'a');
    if(ca != cb) return 0;
  }
  return *a == '\0' && *b == '\0';
}

/* Tiny MIME table sufficient for static sites */
static const char* mime_from_ext(const char* path){
  const char* dot = strrchr(path, '.');
  if(!dot || !dot[1]) return "application/octet-stream";
  const char* ext = dot + 1;
  if (strequal_ci(ext,"html") || strequal_ci(ext,"htm"))  return "text/html; charset=utf-8";
  if (strequal_ci(ext,"css"))   return "text/css; charset=utf-8";
  if (strequal_ci(ext,"js"))    return "application/javascript; charset=utf-8";
  if (strequal_ci(ext,"svg"))   return "image/svg+xml";
  if (strequal_ci(ext,"png"))   return "image/png";
  if (strequal_ci(ext,"jpg") || strequal_ci(ext,"jpeg")) return "image/jpeg";
  if (strequal_ci(ext,"gif"))   return "image/gif";
  if (strequal_ci(ext,"webp"))  return "image/webp";
  if (strequal_ci(ext,"ico"))   return "image/x-icon";
  if (strequal_ci(ext,"woff"))  return "font/woff";
  if (strequal_ci(ext,"woff2")) return "font/woff2";
  if (strequal_ci(ext,"pdf"))   return "application/pdf";
  if (strequal_ci(ext,"mp4"))   return "video/mp4";
  if (strequal_ci(ext,"webm"))  return "video/webm";
  if (strequal_ci(ext,"json"))  return "application/json; charset=utf-8";
  if (strequal_ci(ext,"txt") || strequal_ci(ext,"md"))   return "text/plain; charset=utf-8";
  return "application/octet-stream";
}

/* Small 200/404 helpers keep response formatting in one place */
static void http_send_simple(ueng_socket_t cs, const char* status, const char* body){
  char hdr[512];
  size_t blen = body ? strlen(body) : 0;
  int n = snprintf(hdr, sizeof(hdr),
    "HTTP/1.1 %s\r\n"
    "Content-Type: text/plain; charset=utf-8\r\n"
    "Content-Length: %zu\r\n"
    "Connection: close\r\n\r\n", status, blen);
#ifdef _WIN32
  send(cs, hdr, n, 0);
  if(body && blen) send(cs, body, (int)blen, 0);
#else
  send(cs, hdr, (size_t)n, 0);
  if(body && blen) send(cs, body, blen, 0);
#endif
}

static void http_send_200_head(ueng_socket_t cs, const char* mime, size_t len){
  char hdr[512];
  int n = snprintf(hdr, sizeof(hdr),
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %zu\r\n"
    "Cache-Control: no-cache\r\n"
    "Connection: close\r\n\r\n", mime, len);
#ifdef _WIN32
  send(cs, hdr, n, 0);
#else
  send(cs, hdr, (size_t)n, 0);
#endif
}

/* Stream a file to the client, optionally HEAD-only (no body). */
static int send_file(ueng_socket_t cs, const char* fs_path, const char* mime, int head_only){
  FILE* f = ueng_fopen(fs_path, "rb");
  if(!f) { http_send_simple(cs, "404 Not Found", "404 Not Found\n"); return -1; }
  fseek(f, 0, SEEK_END);
  long sz = ftell(f); if(sz < 0) sz = 0;
  fseek(f, 0, SEEK_SET);

  http_send_200_head(cs, mime, (size_t)sz);
  if(!head_only){
    char buf[16*1024];
    size_t n;
    while((n = fread(buf,1,sizeof(buf),f)) > 0){
#ifdef _WIN32
      int sent = send(cs, (const char*)buf, (int)n, 0);
      if(sent <= 0) break;
#else
      ssize_t sent = send(cs, (const char*)buf, n, 0);
      if(sent <= 0) break;
#endif
    }
  }
  fclose(f);
  return 0;
}

/* Handle a single HTTP/1.1 GET/HEAD request (very small parser). */
static void handle_client(ueng_socket_t cs, const char* root){
  char req[4096];
#ifdef _WIN32
  int rn = recv(cs, req, (int)sizeof(req)-1, 0);
#else
  ssize_t rn = recv(cs, req, sizeof(req)-1, 0);
#endif
  if(rn <= 0) { closesock(cs); return; }
  req[rn] = '\0';

  /* METHOD PATH (very small parse) */
  char method[8]={0}, path_raw[2048]={0};
  if(sscanf(req, "%7s %2047s", method, path_raw) != 2){
    http_send_simple(cs, "400 Bad Request", "400 Bad Request\n"); closesock(cs); return;
  }
  int head_only = 0;
  if(strcmp(method,"GET")==0) head_only = 0;
  else if(strcmp(method,"HEAD")==0) head_only = 1;
  else { http_send_simple(cs, "405 Method Not Allowed", "405 Method Not Allowed\n"); closesock(cs); return; }

  /* Decode %XX encodings; reject bad encodings */
  char path_dec[2048];
  if (url_decode(path_raw, path_dec, sizeof(path_dec)) != 0){
    http_send_simple(cs, "400 Bad Request", "400 Bad Request\n"); closesock(cs); return;
  }

  /* Basic traversal guard after decoding */
  if(path_is_traversal(path_dec)) { http_send_simple(cs, "400 Bad Request", "400 Bad Request\n"); closesock(cs); return; }

  /* Map to filesystem path under root (switch to native separators) */
  char fs_path[4096];
  if(strcmp(path_dec,"/")==0){
    snprintf(fs_path, sizeof(fs_path), "%s%cindex.html", root, PATH_SEP);
  } else {
    const char* p = path_dec[0]=='/' ? path_dec+1 : path_dec;
    char rel[2048]; size_t j=0;
    for(size_t i=0; p[i] && j<sizeof(rel)-1; ++i){
      char c = p[i];
      if(c=='/') c = PATH_SEP;
      rel[j++] = c;
    }
    rel[j] = '\0';
    snprintf(fs_path, sizeof(fs_path), "%s%c%s", root, PATH_SEP, rel);
  }

  if(!file_exists(fs_path)){
    http_send_simple(cs, "404 Not Found", "404 Not Found\n");
    closesock(cs); return;
  }

  const char* mime = mime_from_ext(fs_path);
  (void)send_file(cs, fs_path, mime, head_only);
  closesock(cs);
}

/*-------------------------------- serve_run --------------------------------*/
int serve_run(const char* root, const char* host, int port){
  if(!root || !*root){ fprintf(stderr,"[serve] ERROR: empty root\n"); return 1; }
  /* validate index.html exists before listening */
  {
    char idx[1024]; snprintf(idx,sizeof(idx),"%s%cindex.html", root, PATH_SEP);
    if(!file_exists(idx)){
      fprintf(stderr,"[serve] ERROR: %s not found. Run `uaengine export` or `uaengine build`.\n", idx);
      return 1;
    }
  }

#ifdef _WIN32
  WSADATA wsa;
  if(WSAStartup(MAKEWORD(2,2), &wsa) != 0){
    fprintf(stderr,"[serve] ERROR: WSAStartup failed\n");
    return 1;
  }
  SetConsoleCtrlHandler(ctrl_handler, TRUE);
#else
  signal(SIGINT, SIG_DFL);
  signal(SIGTERM, SIG_DFL);
#endif

  ueng_socket_t s = (ueng_socket_t)socket(AF_INET, SOCK_STREAM, 0);
  if(s < 0){ fprintf(stderr,"[serve] ERROR: socket(): %s\n", strerror(errno));
#ifdef _WIN32
    WSACleanup();
#endif
    return 1; }

  int on = 1;
#ifdef _WIN32
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));
#else
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
#endif

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port   = htons((unsigned short)port);
  if(inet_pton(AF_INET, host, &addr.sin_addr) != 1){
    fprintf(stderr,"[serve] ERROR: invalid host: %s\n", host);
    closesock(s);
#ifdef _WIN32
    WSACleanup();
#endif
    return 1;
  }

  if(bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0){
    fprintf(stderr,"[serve] ERROR: bind(): %s\n", strerror(errno));
    closesock(s);
#ifdef _WIN32
    WSACleanup();
#endif
    return 1;
  }
  if(listen(s, 16) < 0){
    fprintf(stderr,"[serve] ERROR: listen(): %s\n", strerror(errno));
    closesock(s);
#ifdef _WIN32
    WSACleanup();
#endif
    return 1;
  }

  printf("[serve] listening on http://%s:%d/\n", host, port);

  for(;;){
    struct sockaddr_in cli; socklen_t clen = (socklen_t)sizeof(cli);
#ifdef _WIN32
    ueng_socket_t cs = accept(s, (struct sockaddr*)&cli, &clen);
#else
    int cs = accept(s, (struct sockaddr*)&cli, &clen);
#endif
    if(cs < 0){
      /* transient accept error; continue */
      continue;
    }
    handle_client(cs, root);
  }

  /* not reached */
  closesock(s);
#ifdef _WIN32
  WSACleanup();
#endif
  return 0;
}
/*------------------------------ End of file --------------------------------*/