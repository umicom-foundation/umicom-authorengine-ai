/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine) — Minimal CLI in C
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab
 * Date: 14-09-2025
 *
 * PURPOSE
 *   Command-line tool for the Umicom AuthorEngine.
 *   Implemented commands:
 *     - init    : prepare workspace + starter book.yaml
 *     - ingest  : scan dropzone recursively for .md/.markdown/.txt/.pdf and
 *                 draft workspace/outline.md (relative file list)
 *     - build   : read book.yaml, optionally ingest first, normalize chapters
 *                 into workspace/chapters/, create *date-based* outputs,
 *                 CLEAN target (overwrite), write BUILDINFO.txt,
 *                 auto-generate workspace/toc.md and site/index.html
 *     - serve   : tiny static HTTP server to preview any folder (default: site/)
 *
 * DESIGN NOTES
 *   - C-first (C17 baseline), portable; small #ifdefs for Windows/POSIX.
 *   - Safer string handling (snprintf/memcpy), careful allocation.
 *   - No heavy deps: standard C + sockets + simple filesystem ops.
 *---------------------------------------------------------------------------*/

#include <stdio.h>      /* printf, puts, fprintf, FILE, fopen/fclose, fputs, fgets, snprintf */
#include <string.h>     /* strcmp, strlen, memcpy, memmove, strchr, strncmp */
#include <stdlib.h>     /* malloc, free, strtol, qsort, realloc */
#include <errno.h>      /* errno, EEXIST */
#include <ctype.h>      /* isalnum, isspace, tolower, toupper */
#include <time.h>       /* time, gmtime, strftime */
#include "ueng/version.h"

/*---- Forward declarations for commands & helpers --------------------------*/
static int  cmd_init(void);
static int  cmd_ingest(void);
static int  cmd_build(void);
static int  cmd_serve(int argc, char** argv);
static int  cmd_publish(void);
static void usage(void);

static void build_date_utc(char* out, size_t outsz);
static void build_timestamp_utc(char* out, size_t outsz);
static int  write_site_index(const char* site_dir,
                             const char* title,
                             const char* author,
                             const char* slug,
                             const char* stamp);
static int  generate_toc_md(const char* book_title);

/*--- Platform-specific includes for directory & sockets --------------------*/
#ifdef _WIN32
  #define WIN32_LEAN_AND_MEAN
  #include <direct.h>     /* _mkdir */
  #include <io.h>         /* _access */
  #include <winsock2.h>   /* sockets */
  #include <ws2tcpip.h>   /* InetPtonA */
  #include <windows.h>    /* FindFirstFileA, DeleteFileA, RemoveDirectoryA */
  #pragma comment(lib, "ws2_32.lib")
  #define PATH_SEP '\\'
  #define access _access
  #define ACCESS_EXISTS 0
  static int make_dir(const char *path) { return _mkdir(path); }
  typedef SOCKET sock_t;
  #define SOCK_INVALID INVALID_SOCKET
  #define closesock closesocket
#else
  #include <sys/stat.h>   /* mkdir, stat, lstat */
  #include <sys/types.h>
  #include <unistd.h>     /* access, close, read, write, unlink */
  #include <sys/socket.h> /* socket, bind, listen, accept, send, recv */
  #include <netinet/in.h> /* sockaddr_in */
  #include <arpa/inet.h>  /* inet_pton */
  #include <netdb.h>
  #include <dirent.h>     /* opendir, readdir, closedir */
  #define PATH_SEP '/'
  #define ACCESS_EXISTS 0
  static int make_dir(const char *path) { return mkdir(path, 0755); }
  typedef int sock_t;
  #define SOCK_INVALID (-1)
  #define closesock close
#endif

#ifndef _WIN32
  #ifndef FOK
    #define FOK F_OK
  #endif
#endif

/*-----------------------------------------------------------------------------
 * helper_exists_file — 1 if path exists, else 0
 *---------------------------------------------------------------------------*/
static int helper_exists_file(const char *path) {
#ifdef _WIN32
    return (access(path, 0) == ACCESS_EXISTS) ? 1 : 0;
#else
    return (access(path, FOK) == 0) ? 1 : 0;
#endif
}

/*-----------------------------------------------------------------------------
 * ueng_fopen — cross-platform fopen (fopen_s on Windows)
 *---------------------------------------------------------------------------*/
static FILE* ueng_fopen(const char* path, const char* mode) {
#ifdef _WIN32
    FILE* f = NULL;
    return (fopen_s(&f, path, mode) == 0) ? f : NULL;
#else
    return fopen(path, mode);
#endif
}

/*-----------------------------------------------------------------------------
 * ueng_inet_pton4 — cross-platform IPv4 text -> binary
 *---------------------------------------------------------------------------*/
static int ueng_inet_pton4(const char* src, void* dst) {
#ifdef _WIN32
    return (InetPtonA(AF_INET, src, dst) == 1) ? 1 : 0;
#else
    return inet_pton(AF_INET, src, dst);
#endif
}

/*-----------------------------------------------------------------------------
 * trim helpers — remove leading/trailing whitespace (in-place)
 *---------------------------------------------------------------------------*/
static char* ltrim(char* s) { while (*s && isspace((unsigned char)*s)) s++; return s; }
static void  rtrim(char* s) { size_t n=strlen(s); while (n>0 && isspace((unsigned char)s[n-1])) s[--n]='\0'; }
static void  unquote(char* s) {
    size_t n = strlen(s);
    if (n >= 2) {
        char a = s[0], b = s[n-1];
        if ((a=='"' && b=='"') || (a=='\'' && b=='\'')) { memmove(s, s+1, n-2); s[n-2]='\0'; }
    }
}

/*-----------------------------------------------------------------------------
 * mkpath — create all intermediate directories (mkdir -p)
 *---------------------------------------------------------------------------*/
static int mkpath(const char *path) {
    if (!path || !*path) return 0;
    size_t len = strlen(path);
    char *buf = (char*)malloc(len + 1);
    if (!buf) { fprintf(stderr, "[mkpath] ERROR: OOM\n"); return -1; }
    memcpy(buf, path, len);
    buf[len] = '\0';
#ifdef _WIN32
    for (size_t i = 0; i < len; ++i) if (buf[i] == '/') buf[i] = '\\';
#endif
    for (size_t i = 1; i < len; ++i) {
        if (buf[i] == PATH_SEP) {
            char saved = buf[i]; buf[i] = '\0';
            if (!helper_exists_file(buf)) {
                if (make_dir(buf) != 0 && errno != EEXIST) {
                    fprintf(stderr, "[mkpath] WARN: mkdir('%s') failed (errno=%d)\n", buf, errno);
                }
            }
            buf[i] = saved;
        }
    }
    if (!helper_exists_file(buf)) {
        if (make_dir(buf) != 0 && errno != EEXIST) {
            fprintf(stderr, "[mkpath] WARN: mkdir('%s') failed (errno=%d)\n", buf, errno);
            free(buf); return -1;
        }
    }
    free(buf); return 0;
}

/*-----------------------------------------------------------------------------
 * write_text_file_if_absent — create file with content if missing
 *---------------------------------------------------------------------------*/
static int write_text_file_if_absent(const char *path, const char *content) {
    if (helper_exists_file(path)) { printf("[init] skip (exists): %s\n", path); return 0; }
    FILE *f = ueng_fopen(path, "wb");
    if (!f) { fprintf(stderr, "[init] ERROR: cannot open '%s' for writing\n", path); return -1; }
    if (content && *content) fputs(content, f);
    fclose(f);
    printf("[init] wrote: %s\n", path);
    return 0;
}

/*-----------------------------------------------------------------------------
 * write_file — overwrite (or create) file with content
 *---------------------------------------------------------------------------*/
static int write_file(const char *path, const char *content) {
    FILE *f = ueng_fopen(path, "wb");
    if (!f) { fprintf(stderr, "[write] ERROR: cannot open '%s' for writing\n", path); return -1; }
    if (content && *content) fputs(content, f);
    fclose(f);
    return 0;
}

/*-----------------------------------------------------------------------------
 * write_gitkeep — "<dir>/.gitkeep"
 *---------------------------------------------------------------------------*/
static int write_gitkeep(const char *dir) {
    const char *leaf = ".gitkeep";
    int need = snprintf(NULL, 0, "%s%c%s", dir, PATH_SEP, leaf);
    if (need < 0) return -1;
    size_t size = (size_t)need + 1;
    char *path = (char*)malloc(size);
    if (!path) return -1;
    (void)snprintf(path, size, "%s%c%s", dir, PATH_SEP, leaf);
    int rc = write_text_file_if_absent(path, "");
    free(path);
    return rc;
}

/*-----------------------------------------------------------------------------
 * seed_book_yaml — minimal starter manifest (only if book.yaml missing)
 *---------------------------------------------------------------------------*/
static int seed_book_yaml(void) {
    const char *yaml =
        "# Umicom AuthorEngine AI — Book manifest (starter)\n"
        "title: \"My New Book\"\n"
        "author: \"Your Name\"\n"
        "language: \"en-GB\"\n"
        "dropzone: \"dropzone\"\n"
        "images_dir: \"dropzone/images\"\n"
        "target_formats: [pdf, docx, epub, html, md]\n"
        "video_scripts:\n"
        "  enabled: true\n"
        "  lesson_length_minutes: 10\n"
        "  total_lessons: 12\n"
        "site:\n"
        "  enabled: true\n"
        "ingest_on_build: true\n"
        "normalize_chapters_on_build: true\n";
    return write_text_file_if_absent("book.yaml", yaml);
}

/*-----------------------------------------------------------------------------
 * tiny_yaml_get / tiny_yaml_get_bool
 *---------------------------------------------------------------------------*/
static int tiny_yaml_get(const char* filename, const char* key, char* out, size_t outsz) {
    FILE* f = ueng_fopen(filename, "rb");
    if (!f) return -1;
    char line[1024];
    size_t keylen = strlen(key);
    int found = 0;
    while (fgets(line, (int)sizeof(line), f)) {
        char* s = line;
        while (*s && isspace((unsigned char)*s)) s++;
        rtrim(s);
        if (*s == '#' || *s == '\0') continue;
        if (strncmp(s, key, keylen) == 0) {
            const char* p = s + keylen;
            while (*p == ' ' || *p == '\t') p++;
            if (*p == ':') {
                p++;
                while (*p == ' ' || *p == '\t') p++;
                size_t n = strlen(p);
                if (n >= outsz) n = outsz - 1;
                memcpy(out, p, n);
                out[n] = '\0';
                rtrim(out);
                unquote(out);
                found = 1; break;
            }
        }
    }
    fclose(f);
    return found ? 0 : 1;
}
static int tiny_yaml_get_bool(const char* filename, const char* key, int* out) {
    char buf[64];
    int rc = tiny_yaml_get(filename, key, buf, sizeof(buf));
    if (rc != 0) return rc;
    for (char* p = buf; *p; ++p) *p = (char)tolower((unsigned char)*p);
    if (!strcmp(buf, "true") || !strcmp(buf, "yes") || !strcmp(buf, "on") || !strcmp(buf, "1")) { *out = 1; return 0; }
    if (!strcmp(buf, "false")|| !strcmp(buf, "no")  || !strcmp(buf, "off")|| !strcmp(buf, "0")) { *out = 0; return 0; }
    *out = (buf[0] != '\0'); /* fallback: non-empty -> true */
    return 0;
}

/*-----------------------------------------------------------------------------
 * slugify — lower-case, keep [a-z0-9], others -> '-', collapse runs
 *---------------------------------------------------------------------------*/
static void slugify(const char* title, char* out, size_t outsz) {
    size_t j = 0; int prev_dash = 0;
    for (size_t i = 0; title[i] && j + 1 < outsz; ++i) {
        unsigned char c = (unsigned char)title[i];
        if (isalnum(c)) { out[j++] = (char)tolower(c); prev_dash = 0; }
        else            { if (!prev_dash && j > 0) { out[j++] = '-'; prev_dash = 1; } }
    }
    if (j > 0 && out[j-1] == '-') j--;
    out[j] = '\0';
    if (j == 0) snprintf(out, outsz, "%s", "untitled");
}

/*-----------------------------------------------------------------------------
 * build_date_utc / build_timestamp_utc
 *---------------------------------------------------------------------------*/
static void build_date_utc(char* out, size_t outsz) {
    time_t now = time(NULL);
    struct tm g;
#if defined(_WIN32)
    gmtime_s(&g, &now);
#else
    gmtime_r(&now, &g);
#endif
    strftime(out, outsz, "%Y-%m-%d", &g);
}
static void build_timestamp_utc(char* out, size_t outsz) {
    time_t now = time(NULL);
    struct tm g;
#if defined(_WIN32)
    gmtime_s(&g, &now);
#else
    gmtime_r(&now, &g);
#endif
    strftime(out, outsz, "%Y-%m-%dT%H-%M-%SZ", &g);
}

/*-----------------------------------------------------------------------------
 * clean_dir — remove all children of a directory, leaving the dir itself.
 *---------------------------------------------------------------------------*/
#ifdef _WIN32
static int clean_dir(const char* dir) {
    if (!helper_exists_file(dir)) return 0;
    int rc = 0;

    int patn = snprintf(NULL, 0, "%s\\*", dir);
    if (patn < 0) return -1;
    char* pattern = (char*)malloc((size_t)patn + 1);
    if (!pattern) return -1;
    snprintf(pattern, (size_t)patn + 1, "%s\\*", dir);

    WIN32_FIND_DATAA ffd;
    HANDLE h = FindFirstFileA(pattern, &ffd);
    if (h == INVALID_HANDLE_VALUE) { free(pattern); return 0; }

    do {
        const char* name = ffd.cFileName;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        int need = snprintf(NULL, 0, "%s\\%s", dir, name);
        char* child = (char*)malloc((size_t)need + 1);
        if (!child) { rc = -1; break; }
        snprintf(child, (size_t)need + 1, "%s\\%s", dir, name);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (clean_dir(child) != 0) rc = -1;
            SetFileAttributesA(child, FILE_ATTRIBUTE_NORMAL);
            if (!RemoveDirectoryA(child)) rc = -1;
        } else {
            SetFileAttributesA(child, FILE_ATTRIBUTE_NORMAL);
            if (!DeleteFileA(child)) rc = -1;
        }
        free(child);
    } while (FindNextFileA(h, &ffd));

    FindClose(h);
    free(pattern);
    return rc;
}
#else
static int clean_dir(const char* dir) {
    if (!helper_exists_file(dir)) return 0;
    DIR* d = opendir(dir);
    if (!d) return 0;
    int rc = 0;
    struct dirent* ent;
    while ((ent = readdir(d))) {
        const char* name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        int need = snprintf(NULL, 0, "%s/%s", dir, name);
        char* child = (char*)malloc((size_t)need + 1);
        if (!child) { rc = -1; break; }
        snprintf(child, (size_t)need + 1, "%s/%s", dir, name);

        struct stat st;
        if (lstat(child, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (clean_dir(child) != 0) rc = -1;
                if (rmdir(child) != 0) rc = -1;
            } else {
                if (unlink(child) != 0) rc = -1;
            }
        }
        free(child);
    }
    closedir(d);
    return rc;
}
#endif

/*========================= String list + helpers ============================*/
typedef struct {
    char **items;
    size_t count;
    size_t cap;
} StrList;

static void sl_init(StrList *sl) { sl->items=NULL; sl->count=0; sl->cap=0; }
static void sl_free(StrList *sl) {
    if (!sl) return;
    for (size_t i=0;i<sl->count;i++) free(sl->items[i]);
    free(sl->items);
    sl->items=NULL; sl->count=sl->cap=0;
}
static int sl_push(StrList *sl, const char* s) {
    if (sl->count == sl->cap) {
        size_t ncap = sl->cap ? sl->cap*2 : 16;
        char **p = (char**)realloc(sl->items, ncap * sizeof(char*));
        if (!p) return -1;
        sl->items = p; sl->cap = ncap;
    }
    size_t n = strlen(s);
    char *dup = (char*)malloc(n+1);
    if (!dup) return -1;
    memcpy(dup, s, n+1);
    sl->items[sl->count++] = dup;
    return 0;
}

/* Case-insensitive compare */
static int ci_cmp(const char* a, const char* b) {
    for (;;) {
        unsigned char ca = (unsigned char)tolower(*a);
        unsigned char cb = (unsigned char)tolower(*b);
        if (ca < cb) return -1;
        if (ca > cb) return  1;
        if (ca == 0 && cb == 0) return 0;
        a++; b++;
    }
}

/* Natural number, case-insensitive comparator (ch2 < ch10) */
static int natural_ci_cmp(const char* a, const char* b) {
    size_t i=0, j=0;
    for (;;) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[j];

        if (ca == '\0' && cb == '\0') return 0;
        if (isdigit(ca) && isdigit(cb)) {
            /* parse number from a */
            unsigned long long va=0, vb=0;
            size_t ia=i, jb=j;
            while (isdigit((unsigned char)a[ia])) { va = va*10ULL + (unsigned)(a[ia]-'0'); ia++; }
            while (isdigit((unsigned char)b[jb])) { vb = vb*10ULL + (unsigned)(b[jb]-'0'); jb++; }
            if (va < vb) return -1;
            if (va > vb) return  1;
            /* numbers equal -> advance past numeric runs */
            i = ia; j = jb;
            continue;
        }

        /* case-insensitive char compare */
        unsigned char ta = (unsigned char)tolower(ca);
        unsigned char tb = (unsigned char)tolower(cb);
        if (ta < tb) return -1;
        if (ta > tb) return  1;

        if (ca != '\0') i++;
        if (cb != '\0') j++;
    }
}
static int qsort_nat_ci_cmp(const void* A, const void* B) {
    const char* a = *(const char* const*)A;
    const char* b = *(const char* const*)B;
    return natural_ci_cmp(a,b);
}

static int endswith_ic(const char* s, const char* suffix) {
    size_t ns = strlen(s), nx = strlen(suffix);
    if (nx > ns) return 0;
    return ci_cmp(s + (ns - nx), suffix) == 0;
}

/* Normalize rel path to use '/' for Markdown portability */
static void rel_normalize(char* s) {
#ifdef _WIN32
    for (char* p = s; *p; ++p) if (*p == '\\') *p = '/';
#else
    (void)s;
#endif
}
static void rel_to_native_sep(char* s) {
#ifdef _WIN32
    for (char* p = s; *p; ++p) if (*p == '/') *p = '\\';
#else
    (void)s;
#endif
}

/* Recursive directory walk — used by ingest & normalization */
#ifdef _WIN32
static int ingest_walk(const char* abs_dir, const char* rel_dir, StrList* out) {
    int patn = snprintf(NULL, 0, "%s\\*", abs_dir);
    if (patn < 0) return -1;
    char* pattern = (char*)malloc((size_t)patn + 1);
    if (!pattern) return -1;
    snprintf(pattern, (size_t)patn + 1, "%s\\*", abs_dir);

    WIN32_FIND_DATAA ffd;
    HANDLE h = FindFirstFileA(pattern, &ffd);
    if (h == INVALID_HANDLE_VALUE) { free(pattern); return 0; }

    int rc = 0;
    do {
        const char* name = ffd.cFileName;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        int need_abs = snprintf(NULL, 0, "%s\\%s", abs_dir, name);
        char* child_abs = (char*)malloc((size_t)need_abs + 1);
        if (!child_abs) { rc = -1; break; }
        snprintf(child_abs, (size_t)need_abs + 1, "%s\\%s", abs_dir, name);

        int need_rel = rel_dir ? snprintf(NULL, 0, "%s/%s", rel_dir, name)
                               : (int)strlen(name);
        char* child_rel = (char*)malloc((size_t)need_rel + 1);
        if (!child_rel) { free(child_abs); rc = -1; break; }
        if (rel_dir) snprintf(child_rel, (size_t)need_rel + 1, "%s/%s", rel_dir, name);
        else         snprintf(child_rel, (size_t)need_rel + 1, "%s", name);

        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            rc = ingest_walk(child_abs, child_rel, out);
        } else {
            if (endswith_ic(child_rel, ".md") || endswith_ic(child_rel, ".markdown") ||
                endswith_ic(child_rel, ".txt") || endswith_ic(child_rel, ".pdf")) {
                rel_normalize(child_rel);
                rc = sl_push(out, child_rel);
            }
        }

        free(child_abs);
        free(child_rel);
        if (rc != 0) break;
    } while (FindNextFileA(h, &ffd));

    FindClose(h);
    free(pattern);
    return rc;
}
#else
static int ingest_walk(const char* abs_dir, const char* rel_dir, StrList* out) {
    DIR* d = opendir(abs_dir);
    if (!d) return 0;
    int rc = 0;
    struct dirent* ent;
    while ((ent = readdir(d))) {
        const char* name = ent->d_name;
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) continue;

        int need_abs = snprintf(NULL, 0, "%s/%s", abs_dir, name);
        char* child_abs = (char*)malloc((size_t)need_abs + 1);
        if (!child_abs) { rc = -1; break; }
        snprintf(child_abs, (size_t)need_abs + 1, "%s/%s", abs_dir, name);

        int need_rel = rel_dir ? snprintf(NULL, 0, "%s/%s", rel_dir, name)
                               : (int)strlen(name);
        char* child_rel = (char*)malloc((size_t)need_rel + 1);
        if (!child_rel) { free(child_abs); rc = -1; break; }
        if (rel_dir) snprintf(child_rel, (size_t)need_rel + 1, "%s/%s", rel_dir, name);
        else         snprintf(child_rel, (size_t)need_rel + 1, "%s", name);

        struct stat st;
        if (lstat(child_abs, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                rc = ingest_walk(child_abs, child_rel, out);
            } else if (S_ISREG(st.st_mode)) {
                if (endswith_ic(child_rel, ".md") || endswith_ic(child_rel, ".markdown") ||
                    endswith_ic(child_rel, ".txt") || endswith_ic(child_rel, ".pdf")) {
                    rel_normalize(child_rel);
                    rc = sl_push(out, child_rel);
                }
            }
        }

        free(child_abs);
        free(child_rel);
        if (rc != 0) break;
    }
    closedir(d);
    return rc;
}
#endif

/*-----------------------------------------------------------------------------
 * write_outline_md — used by cmd_ingest
 *---------------------------------------------------------------------------*/
static int write_outline_md(const char* title, const char* author,
                            const char* dropzone_rel, const StrList* files) {
    (void)mkpath("workspace");
    char path[512];
    snprintf(path, sizeof(path), "workspace%coutline.md", PATH_SEP);

    size_t est = 1024 + files->count * 256;
    char* buf = (char*)malloc(est);
    if (!buf) return -1;

    char day[32]; build_date_utc(day, sizeof(day));

    int n = snprintf(buf, est,
        "# Draft Outline — %s\n\n"
        "_Author:_ **%s**  \n"
        "_Date:_ **%s**  \n"
        "_Sources scanned:_ `%s`\n\n"
        "## Sources (recursive)\n",
        title, author, day, dropzone_rel ? dropzone_rel : "dropzone");
    if (n < 0) { free(buf); return -1; }
    size_t used = (size_t)n;

    if (files->count == 0) {
        n = snprintf(buf + used, est - used, "\n> No .md/.markdown/.txt/.pdf files found yet.\n");
        if (n < 0) { free(buf); return -1; }
        used += (size_t)n;
    } else {
        for (size_t i=0; i<files->count; ++i) {
            n = snprintf(buf + used, est - used, "- %s\n", files->items[i]);
            if (n < 0) { free(buf); return -1; }
            used += (size_t)n;
            if (used + 256 > est) {
                est *= 2;
                char* tmp = (char*)realloc(buf, est);
                if (!tmp) { free(buf); return -1; }
                buf = tmp;
            }
        }
    }

    n = snprintf(buf + used, est - used,
        "\n---\n"
        "_Tip:_ Add your chapters as **Markdown** files under `%s` and re-run `uaengine ingest`.\n",
        dropzone_rel ? dropzone_rel : "dropzone");
    if (n < 0) { free(buf); return -1; }

    int rc = write_file(path, buf);
    free(buf);
    if (rc == 0) printf("[ingest] wrote: %s\n", path);
    return rc;
}

/*-----------------------------------------------------------------------------
 * cmd_ingest — outline scan of dropzone
 *---------------------------------------------------------------------------*/
static int cmd_ingest(void) {
    char title[256]={0}, author[256]={0}, drop[256]={0};
    if (tiny_yaml_get("book.yaml","title", title,sizeof(title))  != 0) snprintf(title, sizeof(title),  "%s","Untitled");
    if (tiny_yaml_get("book.yaml","author",author,sizeof(author))!= 0) snprintf(author,sizeof(author), "%s","Unknown");
    if (tiny_yaml_get("book.yaml","dropzone",drop,sizeof(drop)) != 0) snprintf(drop, sizeof(drop),    "%s","dropzone");

    if (!helper_exists_file(drop)) {
        fprintf(stderr, "[ingest] ERROR: dropzone path not found: %s\n", drop);
        return 1;
    }
    StrList files; sl_init(&files);
    int rc = ingest_walk(drop, NULL, &files);
    if (rc != 0) { fprintf(stderr, "[ingest] ERROR: directory walk failed\n"); sl_free(&files); return 1; }

    if (files.count > 1) {
        qsort(files.items, files.count, sizeof(char*), qsort_nat_ci_cmp);
    }

    rc = write_outline_md(title, author, drop, &files);
    sl_free(&files);
    if (rc != 0) { fprintf(stderr, "[ingest] ERROR: failed to write outline\n"); return 1; }

    puts("[ingest] complete.");
    return 0;
}

/*======================== Chapter normalization (build) =====================*/
static int ensure_parent_dir(const char* filepath) {
    const char* last_sep = NULL;
    for (const char* p = filepath; *p; ++p) {
        if (*p == PATH_SEP) last_sep = p;
    }
    if (!last_sep) return 0;
    size_t n = (size_t)(last_sep - filepath);
    char* dir = (char*)malloc(n + 1);
    if (!dir) return -1;
    memcpy(dir, filepath, n);
    dir[n] = '\0';
    int rc = mkpath(dir);
    free(dir);
    return rc;
}
static int copy_file_binary(const char* src, const char* dst) {
    FILE* in = ueng_fopen(src, "rb");
    if (!in) { fprintf(stderr, "[copy] open src fail: %s\n", src); return -1; }
    if (ensure_parent_dir(dst) != 0) { fclose(in); fprintf(stderr,"[copy] mkpath fail for %s\n", dst); return -1; }
    FILE* out = ueng_fopen(dst, "wb");
    if (!out) { fclose(in); fprintf(stderr, "[copy] open dst fail: %s\n", dst); return -1; }

    char buf[64*1024];
    size_t rd;
    while ((rd = fread(buf, 1, sizeof(buf), in)) > 0) {
        size_t wr = fwrite(buf, 1, rd, out);
        if (wr != rd) { fclose(in); fclose(out); fprintf(stderr, "[copy] short write: %s\n", dst); return -1; }
    }
    fclose(in);
    fclose(out);
    return 0;
}
static int normalize_chapters(const char* dropzone) {
    StrList files; sl_init(&files);
    char abs[1024]; snprintf(abs, sizeof(abs), "%s", dropzone);
    int rc = ingest_walk(abs, NULL, &files);
    if (rc != 0) { fprintf(stderr, "[normalize] walk failed\n"); sl_free(&files); return 1; }

    if (mkpath("workspace") != 0) fprintf(stderr,"[normalize] WARN: workspace create\n");
    if (helper_exists_file("workspace/chapters")) {
        (void)clean_dir("workspace/chapters");
    } else {
        (void)mkpath("workspace/chapters");
    }

    size_t copied = 0, skipped = 0;
    for (size_t i=0; i<files.count; ++i) {
        const char* rel = files.items[i];
        if (!(endswith_ic(rel,".md") || endswith_ic(rel,".markdown") || endswith_ic(rel,".txt"))) {
            skipped++; continue;
        }

        char rel_native[1024]; snprintf(rel_native, sizeof(rel_native), "%s", rel);
        rel_to_native_sep(rel_native);

        char src[1200];
        snprintf(src, sizeof(src), "%s%c%s", dropzone, PATH_SEP, rel_native);

        char dst[1200];
        snprintf(dst, sizeof(dst), "workspace%cchapters%c%s", PATH_SEP, PATH_SEP, rel_native);

        if (copy_file_binary(src, dst) == 0) copied++;
        else fprintf(stderr,"[normalize] copy failed: %s -> %s\n", src, dst);
    }

    const char* mani = "workspace/chapters/_manifest.txt";
    char line[256];
    int n = snprintf(line, sizeof(line), "copied=%u skipped=%u\n", (unsigned)copied, (unsigned)skipped);
    if (n > 0) (void)write_file(mani, line);

    printf("[normalize] chapters: copied %u, skipped %u\n", (unsigned)copied, (unsigned)skipped);

    sl_free(&files);
    return 0;
}

/*============================ TOC generation =================================*/

/* Turn filename into a friendlier label: drop extension, replace _/- with space,
   collapse spaces, Title Case. Also map leading "chNN" to "Chapter NN — ..." */
static void titlecase_inplace(char* s) {
    int new_word = 1;
    for (size_t i=0; s[i]; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (isalpha(c)) {
            s[i] = (char)(new_word ? toupper(c) : tolower(c));
            new_word = 0;
        } else if (isdigit(c)) {
            new_word = 0;
        } else {
            /* any non-alnum -> space */
            s[i] = ' ';
            new_word = 1;
        }
    }
    /* collapse multiple spaces in-place */
    size_t w=0;
    for (size_t r=0; s[r]; ++r) {
        if (s[r] == ' ' && (w==0 || s[w-1]==' ')) continue;
        s[w++] = s[r];
    }
    if (w>0 && s[w-1]==' ') w--;
    s[w]='\0';
}

static void make_label_from_filename(const char* filename, char* out, size_t outsz) {
    /* get base name (after last '/') */
    const char* base = filename;
    const char* slash = strrchr(filename, '/');
    if (slash) base = slash + 1;

    /* copy and strip extension */
    size_t n = strlen(base);
    if (n >= outsz) n = outsz - 1;
    memcpy(out, base, n);
    out[n] = '\0';
    char* dot = strrchr(out, '.');
    if (dot) *dot = '\0';

    titlecase_inplace(out);

    /* Special: "ChNN ..." => "Chapter NN — ..." */
    if ((out[0]=='C' || out[0]=='c') && (out[1]=='h' || out[1]=='H')) {
        size_t i=2; while (out[i]==' ') i++;
        size_t j=i; unsigned num=0; int had_digit=0;
        while (isdigit((unsigned char)out[j])) { had_digit=1; num = num*10u + (unsigned)(out[j]-'0'); j++; }
        if (had_digit) {
            /* Build "Chapter <num> — <rest>" */
            char rest[256]; size_t k=0;
            while (out[j]==' ') j++;
            for (; out[j] && k+1<sizeof(rest); ++j) rest[k++] = out[j];
            rest[k]='\0';

            char tmp[512];
            if (rest[0]) snprintf(tmp, sizeof(tmp), "Chapter %u — %s", num, rest);
            else         snprintf(tmp, sizeof(tmp), "Chapter %u", num);
            snprintf(out, outsz, "%s", tmp);
        }
    }
}

/* Create workspace/toc.md by scanning workspace/chapters (skip PDFs) */
static int generate_toc_md(const char* book_title) {
    const char* root = "workspace/chapters";
    if (!helper_exists_file(root)) {
        /* nothing to do (no chapters yet) */
        return 0;
    }

    StrList files; sl_init(&files);
    int rc = ingest_walk(root, NULL, &files);
    if (rc != 0) { sl_free(&files); return 1; }

    /* keep only .md/.markdown/.txt for TOC */
    /* also prefix 'chapters/' to create link targets */
    StrList kept; sl_init(&kept);
    for (size_t i=0; i<files.count; ++i) {
        const char* rel = files.items[i];
        if (endswith_ic(rel,".pdf")) continue;

        /* normalize already '/', just ensure we're "chapters/rel" */
        int need = snprintf(NULL, 0, "chapters/%s", rel);
        if (need < 0) { sl_free(&files); sl_free(&kept); return 1; }
        char* path = (char*)malloc((size_t)need + 1);
        if (!path) { sl_free(&files); sl_free(&kept); return 1; }
        snprintf(path, (size_t)need + 1, "chapters/%s", rel);
        if (sl_push(&kept, path) != 0) { free(path); sl_free(&files); sl_free(&kept); return 1; }
        free(path);
    }
    sl_free(&files);

    if (kept.count > 1) {
        qsort(kept.items, kept.count, sizeof(char*), qsort_nat_ci_cmp);
    }

    (void)mkpath("workspace");
    const char* outpath = "workspace/toc.md";

    /* Estimate size and build content */
    size_t est = 1024 + kept.count * 160;
    char* buf = (char*)malloc(est);
    if (!buf) { sl_free(&kept); return 1; }

    int n = snprintf(buf, est,
        "# Table of Contents — %s\n\n"
        "> Draft TOC generated from `workspace/chapters/`.\n\n",
        (book_title && *book_title) ? book_title : "Untitled");
    if (n < 0) { free(buf); sl_free(&kept); return 1; }
    size_t used = (size_t)n;

    if (kept.count == 0) {
        n = snprintf(buf + used, est - used, "_No chapters found yet._\n");
        if (n < 0) { free(buf); sl_free(&kept); return 1; }
        used += (size_t)n;
    } else {
        for (size_t i=0; i<kept.count; ++i) {
            const char* link = kept.items[i];
            /* label from filename */
            char label[256];
            make_label_from_filename(link, label, sizeof(label));

            n = snprintf(buf + used, est - used, "- [%s](<%s>)\n", label, link);
            if (n < 0) { free(buf); sl_free(&kept); return 1; }
            used += (size_t)n;

            if (used + 256 > est) {
                est *= 2;
                char* tmp = (char*)realloc(buf, est);
                if (!tmp) { free(buf); sl_free(&kept); return 1; }
                buf = tmp;
            }
        }
    }

    int wr = write_file(outpath, buf);
    free(buf);
    sl_free(&kept);

    if (wr == 0) printf("[toc] wrote: %s\n", outpath);
    else         fprintf(stderr, "[toc] ERROR: failed to write %s\n", outpath);

    return wr == 0 ? 0 : 1;
}

/*=============================== INIT ======================================*/
static int cmd_init(void) {
    const char *dirs[] = {
        "dropzone","dropzone/images","workspace","outputs",
        "templates","prompts","themes", NULL
    };
    for (int i = 0; dirs[i] != NULL; ++i) {
        if (mkpath(dirs[i]) == 0) { printf("[init] ok: %s\n", dirs[i]); (void)write_gitkeep(dirs[i]); }
        else { fprintf(stderr, "[init] ERROR: could not create path: %s\n", dirs[i]); }
    }
    if (seed_book_yaml() != 0) { fprintf(stderr, "[init] ERROR: failed to write book.yaml\n"); return 1; }
    puts("[init] complete."); return 0;
}

/*=============================== BUILD =====================================*/
static int write_site_index(const char* site_dir,
                            const char* title,
                            const char* author,
                            const char* slug,
                            const char* stamp) {
    char path[1024];
    if (snprintf(path, sizeof(path), "%s%cindex.html", site_dir, PATH_SEP) < 0) return -1;

    const char* tpl =
"<!doctype html>\n"
"<html lang=\"en\">\n"
"<head>\n"
"  <meta charset=\"utf-8\">\n"
"  <meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">\n"
"  <title>%s — Umicom AuthorEngine AI</title>\n"
"  <style>\n"
"    body{font-family:system-ui,-apple-system,\"Segoe UI\",Roboto,Ubuntu,sans-serif;margin:2rem;line-height:1.5}\n"
"    .card{border:1px solid #ddd;border-radius:12px;padding:1.5rem;max-width:800px}\n"
"    code{background:#f6f8fa;padding:2px 6px;border-radius:6px}\n"
"    .muted{color:#666}\n"
"  </style>\n"
"</head>\n"
"<body>\n"
"  <div class=\"card\">\n"
"    <h1>%s</h1>\n"
"    <p class=\"muted\">by %s</p>\n"
"    <p><strong>Slug:</strong> <code>%s</code><br><strong>Build:</strong> <code>%s</code></p>\n"
"    <p>This site was generated by <strong>Umicom AuthorEngine AI</strong>. Replace this page with your own content during the render stage.</p>\n"
"  </div>\n"
"</body>\n"
"</html>\n";
    int need = snprintf(NULL, 0, tpl, title, title, author, slug, stamp);
    if (need < 0) return -1;
    size_t size = (size_t)need + 1;

    char* html = (char*)malloc(size);
    if (!html) return -1;
    (void)snprintf(html, size, tpl, title, title, author, slug, stamp);
    if (write_file(path, html) != 0) { free(html); return -1; }
    free(html);
    return 0;
}

static int cmd_build(void) {
    /* 1) Load metadata from book.yaml */
    char title[256]={0}, author[256]={0}; int rc;
    rc = tiny_yaml_get("book.yaml","title", title,sizeof(title));
    if (rc == -1) { fprintf(stderr,"[build] ERROR: cannot open book.yaml (run `uaengine init`)\n"); return 1; }
    if (rc ==  1) { snprintf(title,sizeof(title),"%s","Untitled"); }
    rc = tiny_yaml_get("book.yaml","author",author,sizeof(author));
    if (rc == -1) { fprintf(stderr,"[build] ERROR: cannot open book.yaml\n"); return 1; }
    if (rc ==  1) { snprintf(author,sizeof(author),"%s","Unknown"); }

    /* 2) Optionally run the ingestor first */
    int ingest_first = 0;
    if (tiny_yaml_get_bool("book.yaml", "ingest_on_build", &ingest_first) == 0 && ingest_first) {
        puts("[build] ingest_on_build: true — running ingest...");
        if (cmd_ingest() != 0) {
            fprintf(stderr, "[build] WARN: ingest failed; continuing build.\n");
        }
    }

    /* 3) Optionally normalize chapters into workspace/chapters */
    int norm = 0;
    char drop[256]={0};
    if (tiny_yaml_get("book.yaml","dropzone",drop,sizeof(drop)) != 0) snprintf(drop,sizeof(drop),"%s","dropzone");
    if (tiny_yaml_get_bool("book.yaml","normalize_chapters_on_build",&norm) == 0 && norm) {
        printf("[build] normalize_chapters_on_build: true — mirroring from %s\n", drop);
        if (normalize_chapters(drop) != 0) {
            fprintf(stderr,"[build] WARN: chapter normalization failed; continuing.\n");
        }
    }

    /* 4) Generate a draft workspace/toc.md from normalized chapters */
    (void)generate_toc_md(title);

    /* 5) Compute slug/date/stamp and prepare outputs/<slug>/<YYYY-MM-DD> */
    char slug[256]; slugify(title, slug, sizeof(slug));
    char day[32];   build_date_utc(day, sizeof(day));
    char stamp[64]; build_timestamp_utc(stamp, sizeof(stamp));

    char root[512];
    snprintf(root,sizeof(root),"outputs%c%s%c%s",PATH_SEP,slug,PATH_SEP,day);

    if (helper_exists_file(root)) {
        printf("[build] cleaning existing: %s\n", root);
        if (clean_dir(root) != 0) {
            fprintf(stderr,"[build] WARN: could not fully clean %s (some files may remain)\n", root);
        }
    } else {
        if (mkpath(root) != 0) { fprintf(stderr,"[build] ERROR: cannot create %s\n", root); return 1; }
    }
    (void)mkpath(root);

    const char *sub[] = {"pdf","docx","epub","html","md","cover","video-scripts","site",NULL};
    for (int i=0; sub[i]!=NULL; ++i) {
        char p[640];
        snprintf(p,sizeof(p),"%s%c%s",root,PATH_SEP,sub[i]);
        if (mkpath(p)!=0) fprintf(stderr,"[build] WARN: cannot create %s\n",p);
    }

    char info_path[640]; snprintf(info_path,sizeof(info_path),"%s%cBUILDINFO.txt",root,PATH_SEP);
    char info[1024];
    int n = snprintf(info,sizeof(info),
        "Title:  %s\nAuthor: %s\nSlug:   %s\nDate:   %s\nStamp:  %s\n",
        title,author,slug,day,stamp);
    if (n<0 || n>=(int)sizeof(info)) fprintf(stderr,"[build] WARN: info truncated\n");
    if (write_file(info_path, info)!=0) { fprintf(stderr,"[build] ERROR: cannot write BUILDINFO.txt\n"); return 1; }

    char site_dir[640]; snprintf(site_dir,sizeof(site_dir),"%s%c%s",root,PATH_SEP,"site");
    if (write_site_index(site_dir, title, author, slug, stamp) != 0) {
        fprintf(stderr,"[build] WARN: could not write site/index.html\n");
    }

    printf("[build] ok: %s\n", root);
    puts("[build] outputs will be overwritten on subsequent builds for the same date.");
    return 0;
}

/*============================== SERVE ======================================*/
static const char* serve_guess_mime(const char* path) {
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
static int serve_build_fs_path(char* out, size_t outsz, const char* root, const char* uri) {
    if (!uri || !*uri) uri = "/";
    if (strstr(uri, "..")) return -1;
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
static int serve_send_all(sock_t s, const char* buf, size_t len) {
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
static void serve_handle_client(sock_t cs, const char* root) {
    char req[2048]; memset(req, 0, sizeof(req));
#ifdef _WIN32
    int n = recv(cs, req, (int)sizeof(req)-1, 0);
#else
    int n = (int)recv(cs, req, sizeof(req)-1, 0);
#endif
    if (n <= 0) { closesock(cs); return; }

    char method[8]={0}, uri[1024]={0};
#ifdef _WIN32
    if (sscanf_s(req, "%7s %1023s", method, (unsigned)sizeof(method), uri, (unsigned)sizeof(uri)) != 2
        || strcmp(method,"GET")!=0) {
#else
    if (sscanf(req, "%7s %1023s", method, uri) != 2 || strcmp(method,"GET")!=0) {
#endif
        const char* m = "HTTP/1.0 405 Method Not Allowed\r\nConnection: close\r\n\r\n";
        (void)serve_send_all(cs, m, strlen(m));
        closesock(cs); return;
    }

    char path[1024];
    if (serve_build_fs_path(path, sizeof(path), root, uri) != 0) {
        const char* m = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\n\r\n";
        (void)serve_send_all(cs, m, strlen(m)); closesock(cs); return;
    }

    FILE* f = ueng_fopen(path, "rb");
    if (!f) {
        const char* m404 = "HTTP/1.0 404 Not Found\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\n404 Not Found\n";
        (void)serve_send_all(cs, m404, strlen(m404)); closesock(cs); return;
    }

    if (fseek(f, 0, SEEK_END) != 0) { fclose(f); closesock(cs); return; }
    long sz = ftell(f); if (sz < 0) { fclose(f); closesock(cs); return; }
    rewind(f);

    char* buf = (char*)malloc((size_t)sz);
    if (!buf) { fclose(f); closesock(cs); return; }
    size_t rd = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (rd != (size_t)sz) { free(buf); closesock(cs); return; }

    char header[512];
    const char* mime = serve_guess_mime(path);
    int h = snprintf(header, sizeof(header),
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n", mime, sz);
    if (h < 0 || h >= (int)sizeof(header)) { free(buf); closesock(cs); return; }

    (void)serve_send_all(cs, header, (size_t)h);
    (void)serve_send_all(cs, buf, (size_t)sz);
    free(buf);
    closesock(cs);
}

/*=========================== usage & dispatch ===============================*/
static void usage(void) {
    printf("uaengine %s\n", UENG_VERSION);
    printf("Usage: uaengine <init|ingest|build|serve|publish|--version>\n");
}
static int cmd_publish(void) { puts("[publish] repo (TODO)"); return 0; }

/* The serve command: uaengine serve <dir> [port] */
static int cmd_serve(int argc, char** argv) {
    if (argc < 3) {
        puts("Usage: uaengine serve <folder-to-serve> [port]");
        puts("Example: uaengine serve outputs/my-book/2025-09-14/site 8080");
        return 1;
    }
    const char* root = argv[2];
    int port = 8080;
    if (argc >= 4) {
        long p = strtol(argv[3], NULL, 10);
        if (p > 0 && p < 65536) port = (int)p;
    }
    if (!helper_exists_file(root)) {
        fprintf(stderr, "[serve] ERROR: folder does not exist: %s\n", root);
        return 1;
    }

#ifdef _WIN32
    WSADATA w; if (WSAStartup(MAKEWORD(2,2), &w) != 0) { fprintf(stderr, "[serve] ERROR: WSAStartup failed\n"); return 1; }
#endif

    sock_t s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == SOCK_INVALID) { fprintf(stderr, "[serve] ERROR: socket()\n"); goto cleanup_ws; }

    int opt=1;
#ifdef _WIN32
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
#else
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    struct sockaddr_in addr; memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((unsigned short)port);
    if (ueng_inet_pton4("127.0.0.1", &addr.sin_addr) != 1) {
        fprintf(stderr, "[serve] ERROR: inet_pton failed\n");
        closesock(s); goto cleanup_ws;
    }

    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
        fprintf(stderr, "[serve] ERROR: bind() failed (port %d). Is it in use?\n", port);
        closesock(s); goto cleanup_ws;
    }
    if (listen(s, 16) != 0) {
        fprintf(stderr, "[serve] ERROR: listen() failed\n");
        closesock(s); goto cleanup_ws;
    }

    printf("[serve] Serving %s at http://127.0.0.1:%d\n", root, port);
    printf("[serve] Press Ctrl+C to stop.\n");

    for (;;) {
        struct sockaddr_in cli; socklen_t clen = (socklen_t)sizeof(cli);
        sock_t cs = accept(s, (struct sockaddr*)&cli, &clen);
        if (cs == SOCK_INVALID) continue;
        serve_handle_client(cs, root);
    }

    closesock(s);
cleanup_ws:
#ifdef _WIN32
    WSACleanup();
#endif
    return 0;
}

/* Entry */
int main(int argc, char **argv) {
    if (argc < 2) { usage(); return 0; }
    const char *cmd = argv[1];
    if      (strcmp(cmd, "init")    == 0) return cmd_init();
    else if (strcmp(cmd, "ingest")  == 0) return cmd_ingest();
    else if (strcmp(cmd, "build")   == 0) return cmd_build();
    else if (strcmp(cmd, "serve")   == 0) return cmd_serve(argc, argv);
    else if (strcmp(cmd, "publish") == 0) return cmd_publish();
    else if ((strcmp(cmd, "-v") == 0) || (strcmp(cmd, "--version") == 0)) { puts(UENG_VERSION); return 0; }
    else { fprintf(stderr, "Unknown command: %s\n", cmd); usage(); return 1; }
}
