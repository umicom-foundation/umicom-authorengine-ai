/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * File: src/serve.c
 * Purpose: Tiny cross-platform static file HTTP server implementation
 *
 * Notes:
 *  - Serves GET/HEAD only
 *  - Prevents path traversal ('..' and '%')
 *  - Guesses a few common MIME types
 *  - On Windows, performs WSAStartup / WSACleanup
 *
 * Copyright (c) 2025 Umicom Foundation
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab + contributors
 * License: MIT (see LICENSE)
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET sock_t;
  #define closesock closesocket
  #define SOCK_INVALID INVALID_SOCKET
  #define PATH_SEP '\\'
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/stat.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <netdb.h>
  #include <unistd.h>
  typedef int sock_t;
  #define closesock close
  #define SOCK_INVALID (-1)
  #define PATH_SEP '/'
#endif

#include "ueng/serve.h"

/*----------------------------- helpers -------------------------------------*/
static const char* guess_mime(const char* path) {
    const char *dot = strrchr(path, '.');
    if (!dot) return "application/octet-stream";
    if (!strcmp(dot,".html")||!strcmp(dot,".htm")) return "text/html; charset=utf-8";
    if (!strcmp(dot,".css"))  return "text/css; charset=utf-8";
    if (!strcmp(dot,".js"))   return "application/javascript";
    if (!strcmp(dot,".json")) return "application/json";
    if (!strcmp(dot,".svg"))  return "image/svg+xml";
    if (!strcmp(dot,".png"))  return "image/png";
    if (!strcmp(dot,".jpg")||!strcmp(dot,".jpeg")) return "image/jpeg";
    if (!strcmp(dot,".gif"))  return "image/gif";
    if (!strcmp(dot,".ico"))  return "image/x-icon";
    if (!strcmp(dot,".txt")||!strcmp(dot,".md")) return "text/plain; charset=utf-8";
    return "application/octet-stream";
}

static int send_all(sock_t s, const char* buf, size_t len) {
    size_t sent = 0;
    while (sent < len) {
#ifdef _WIN32
        int n = send(s, buf + sent, (int)(len - sent), 0);
#else
        int n = (int)send(s, buf + sent, len - sent, 0);
#endif
        if (n <= 0) return -1;
        sent += (size_t)n;
    }
    return 0;
}

static void send_404(sock_t cs) {
    const char* msg = "HTTP/1.0 404 Not Found\r\n"
                      "Content-Type: text/plain; charset=utf-8\r\n"
                      "Connection: close\r\n\r\n"
                      "Not Found\n";
    (void)send_all(cs, msg, strlen(msg));
}

static int build_fs_path(char* out, size_t outsz, const char* root, const char* uri) {
    if (!uri || !*uri) uri = "/";
    if (strstr(uri, "..")) return -1;
    if (strchr(uri, '%'))  return -1; /* reject encoded traversal */
    size_t ulen = strcspn(uri, "?\r\n");
    int need_index = (ulen == 1 && uri[0] == '/');

    char rel[512]; size_t j = 0;
    for (size_t i = 0; i < ulen && j + 1 < sizeof(rel); ++i) {
        char c = uri[i];
        if (i==0 && c=='/') continue;
#ifdef _WIN32
        if (c == '/') c = '\\';
#endif
        rel[j++] = c;
    }
    rel[j] = '\0';
    if (need_index) snprintf(rel, sizeof(rel), "index.html");

    int need = snprintf(NULL, 0, "%s%c%s", root, PATH_SEP, rel);
    if (need < 0 || (size_t)need + 1 > outsz) return -1;
    snprintf(out, outsz, "%s%c%s", root, PATH_SEP, rel);
    return 0;
}

static void handle_client(sock_t cs, const char* root) {
    char req[2048]; memset(req, 0, sizeof(req));
#ifdef _WIN32
    int n = recv(cs, req, (int)sizeof(req)-1, 0);
#else
    int n = (int)recv(cs, req, sizeof(req)-1, 0);
#endif
    if (n <= 0) { closesock(cs); return; }

    char method[8] = {0};
    char uri[1024] = {0};

#ifdef _WIN32
    if (sscanf_s(req, "%7s %1023s", method, (unsigned)sizeof(method),
                 uri, (unsigned)sizeof(uri)) != 2 ||
        (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)) {
#else
    if (sscanf(req, "%7s %1023s", method, uri) != 2 ||
        (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)) {
#endif
        send_404(cs);
        closesock(cs);
        return;
    }

    if (strchr(uri, '%')) { send_404(cs); closesock(cs); return; }

    char path[1024];
    if (build_fs_path(path, sizeof(path), root, uri) != 0) {
        send_404(cs); closesock(cs); return;
    }

    FILE* f = NULL;
#ifdef _WIN32
    if (fopen_s(&f, path, "rb")) { send_404(cs); closesock(cs); return; }
#else
    f = fopen(path, "rb");
    if (!f) { send_404(cs); closesock(cs); return; }
#endif

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); closesock(cs); return; }
    long sz = ftell(f); if (sz < 0) { fclose(f); closesock(cs); return; }
    rewind(f);

    const char* mime = guess_mime(path);
    char header[512];
    int h = snprintf(header, sizeof(header),
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "X-Content-Type-Options: nosniff\r\n"
        "Connection: close\r\n\r\n",
        mime, sz);
    if (h < 0 || h >= (int)sizeof(header)) { fclose(f); closesock(cs); return; }

    if (send_all(cs, header, (size_t)h) != 0) { fclose(f); closesock(cs); return; }
    if (strcmp(method, "HEAD") == 0) { fclose(f); closesock(cs); return; }

    char buf[65536];
    size_t nrd;
    while ((nrd = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (send_all(cs, buf, nrd) != 0) { fclose(f); closesock(cs); return; }
    }

    fclose(f);
    closesock(cs);
}

/*------------------------------- public API --------------------------------*/
int serve_run(const char* root, const char* host, int port) {
#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        fprintf(stderr, "[serve] ERROR: WSAStartup failed\n");
        return 1;
    }
#endif

    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == SOCK_INVALID) {
        fprintf(stderr, "[serve] ERROR: socket()\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    int yes = 1;
#ifdef _WIN32
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));
#else
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
#endif

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((unsigned short)port);
    if (host && strcmp(host,"0.0.0.0") != 0) {
        if (inet_pton(AF_INET, host, &addr.sin_addr) != 1) {
            fprintf(stderr, "[serve] ERROR: invalid host '%s'\n", host ? host : "(null)");
            closesock(s);
#ifdef _WIN32
            WSACleanup();
#endif
            return 1;
        }
    } else {
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
    }

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        fprintf(stderr, "[serve] ERROR: bind() on port %d\n", port);
        closesock(s);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    if (listen(s, 64) != 0) {
        fprintf(stderr, "[serve] ERROR: listen()\n");
        closesock(s);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    printf("[serve] Serving %s at http://%s:%d\n", root, host ? host : "0.0.0.0", port);

    for (;;) {
        struct sockaddr_in cli;
        socklen_t clen = (socklen_t)sizeof(cli);
        sock_t cs = accept(s, (struct sockaddr*)&cli, &clen);
        if (cs == SOCK_INVALID) {
            /* transient error; continue */
            continue;
        }
        handle_client(cs, root);
    }

    /* unreachable in current CLI design */
    closesock(s);
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}
/*------------------------------- end of file ------------------------------*/
