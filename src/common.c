#include "ueng/common.h"

#ifdef _WIN32
  #ifndef FOK
    #define FOK 0
  #endif
#else
  #ifndef FOK
    #define FOK F_OK
  #endif
  #include <dirent.h>
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <limits.h>
#endif

/* ---------------- file exists + fopen ---------------- */
int ueng_file_exists(const char* path) {
#ifdef _WIN32
  return (access(path, 0) == ACCESS_EXISTS) ? 1 : 0;
#else
  return (access(path, FOK) == 0) ? 1 : 0;
#endif
}

FILE* ueng_fopen(const char* path, const char* mode) {
#ifdef _WIN32
  FILE* f = NULL;
  return (fopen_s(&f, path, mode) == 0) ? f : NULL;
#else
  return fopen(path, mode);
#endif
}

/* ---------------- inet pton v4 ---------------- */
int ueng_inet_pton4(const char* src, void* dst) {
#ifdef _WIN32
  return (InetPtonA(AF_INET, src, dst) == 1) ? 1 : 0;
#else
  return inet_pton(AF_INET, src, dst);
#endif
}

/* ---------------- string helpers --------------- */
char* ueng_ltrim(char* s) {
  while (*s && isspace((unsigned char)*s)) s++;
  return s;
}
void ueng_rtrim(char* s) {
  size_t n = strlen(s);
  while (n > 0 && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
}
void ueng_unquote(char* s) {
  size_t n = strlen(s);
  if (n >= 2) {
    char a = s[0], b = s[n-1];
    if ((a=='"' && b=='"') || (a=='\'' && b=='\'')) {
      memmove(s, s+1, n-2);
      s[n-2] = '\0';
    }
  }
}

/* ---------------- mkpath / clean_dir ----------- */
int ueng_mkpath(const char* path) {
  if (!path || !*path) return 0;
  size_t len = strlen(path);
  char* buf = (char*)malloc(len + 1);
  if (!buf) return -1;
  memcpy(buf, path, len);
  buf[len] = '\0';
#ifdef _WIN32
  for (size_t i=0;i<len;i++) if (buf[i]=='/') buf[i] = '\\';
#endif
  for (size_t i=1;i<len;i++) {
    if (buf[i] == PATH_SEP) {
      char save = buf[i]; buf[i] = '\0';
      if (!ueng_file_exists(buf)) {
        if (ueng_make_dir(buf) != 0 && errno != EEXIST) { /* best-effort */ }
      }
      buf[i] = save;
    }
  }
  if (!ueng_file_exists(buf)) {
    if (ueng_make_dir(buf) != 0 && errno != EEXIST) { free(buf); return -1; }
  }
  free(buf);
  return 0;
}

#ifdef _WIN32
int ueng_clean_dir(const char* dir) {
  if (!ueng_file_exists(dir)) return 0;
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
    if (!strcmp(name, ".") || !strcmp(name, "..")) continue;

    int need = snprintf(NULL, 0, "%s\\%s", dir, name);
    char* child = (char*)malloc((size_t)need + 1);
    if (!child) { rc = -1; break; }
    snprintf(child, (size_t)need + 1, "%s\\%s", dir, name);

    if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
      if (ueng_clean_dir(child) != 0) rc = -1;
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
#include <dirent.h>
int ueng_clean_dir(const char* dir) {
  if (!ueng_file_exists(dir)) return 0;
  DIR* d = opendir(dir);
  if (!d) return 0;
  int rc = 0;
  struct dirent* ent;
  while ((ent = readdir(d))) {
    const char* name = ent->d_name;
    if (!strcmp(name, ".") || !strcmp(name, "..")) continue;

    int need = snprintf(NULL, 0, "%s/%s", dir, name);
    char* child = (char*)malloc((size_t)need + 1);
    if (!child) { rc = -1; break; }
    snprintf(child, (size_t)need + 1, "%s/%s", dir, name);

    struct stat st;
    if (lstat(child, &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        if (ueng_clean_dir(child) != 0) rc = -1;
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

/* ---------------- relative path normalizers --- */
void ueng_rel_normalize(char* s) {
#ifdef _WIN32
  for (char* p = s; *p; ++p) if (*p == '\\') *p = '/';
#else
  (void)s;
#endif
}
void ueng_rel_to_native_sep(char* s) {
#ifdef _WIN32
  for (char* p = s; *p; ++p) if (*p == '/') *p = '\\';
#else
  (void)s;
#endif
}

/* ---------------- endswith (icase) ------------- */
static int ueng_ci_cmp(const char* a, const char* b) {
  for (;;) {
    unsigned char ca = (unsigned char)tolower(*a);
    unsigned char cb = (unsigned char)tolower(*b);
    if (ca < cb) return -1;
    if (ca > cb) return  1;
    if (ca == 0 && cb == 0) return 0;
    a++; b++;
  }
}
int ueng_endswith_ic(const char* s, const char* suffix) {
  size_t ns = strlen(s), nx = strlen(suffix);
  if (nx > ns) return 0;
  return ueng_ci_cmp(s + (ns - nx), suffix) == 0;
}

/* ---------------- StrList ---------------------- */
void ueng_sl_init(UengStrList* sl){ sl->items=NULL; sl->count=0; sl->cap=0; }
void ueng_sl_free(UengStrList* sl){
  if (!sl) return;
  for (size_t i=0;i<sl->count;i++) free(sl->items[i]);
  free(sl->items);
  sl->items=NULL; sl->count=sl->cap=0;
}
int ueng_sl_push(UengStrList* sl, const char* s){
  if (sl->count == sl->cap) {
    size_t ncap = sl->cap ? sl->cap*2 : 16;
    char** p = (char**)realloc(sl->items, ncap * sizeof(char*));
    if (!p) return -1;
    sl->items = p; sl->cap = ncap;
  }
  size_t n = strlen(s);
  char* dup = (char*)malloc(n+1);
  if (!dup) return -1;
  memcpy(dup, s, n+1);
  sl->items[sl->count++] = dup;
  return 0;
}

/* natural, case-insensitive compare (for qsort) */
static int ueng_nat_ci_cmp(const char* a, const char* b) {
  size_t i=0, j=0;
  for (;;) {
    unsigned char ca = (unsigned char)a[i];
    unsigned char cb = (unsigned char)b[j];
    if (ca == '\0' && cb == '\0') return 0;
    if (isdigit(ca) && isdigit(cb)) {
      unsigned long long va=0, vb=0;
      size_t ia=i, jb=j;
      while (isdigit((unsigned char)a[ia])) { va = va*10ULL + (unsigned)(a[ia]-'0'); ia++; }
      while (isdigit((unsigned char)b[jb])) { vb = vb*10ULL + (unsigned)(b[jb]-'0'); jb++; }
      if (va < vb) return -1;
      if (va > vb) return  1;
      i = ia; j = jb; continue;
    }
    unsigned char ta = (unsigned char)tolower(ca);
    unsigned char tb = (unsigned char)tolower(cb);
    if (ta < tb) return -1;
    if (ta > tb) return  1;
    if (ca != '\0') i++;
    if (cb != '\0') j++;
  }
}
int ueng_qsort_nat_ci_cmp(const void* A, const void* B) {
  const char* a = *(const char* const*)A;
  const char* b = *(const char* const*)B;
  return ueng_nat_ci_cmp(a,b);
}

/* ---------------- slug + dates ----------------- */
void ueng_slugify(const char* title, char* out, size_t outsz) {
  size_t j = 0; int prev_dash = 0;
  for (size_t i=0; title[i] && j + 1 < outsz; ++i) {
    unsigned char c = (unsigned char)title[i];
    if (isalnum(c)) { out[j++] = (char)tolower(c); prev_dash = 0; }
    else            { if (!prev_dash && j > 0) { out[j++] = '-'; prev_dash = 1; } }
  }
  if (j > 0 && out[j-1] == '-') j--;
  out[j] = '\0';
  if (j == 0) snprintf(out, outsz, "%s", "untitled");
}

void ueng_build_date_utc(char* out, size_t outsz) {
  time_t now = time(NULL);
  struct tm g;
#if defined(_WIN32)
  gmtime_s(&g, &now);
#else
  gmtime_r(&now, &g);
#endif
  strftime(out, outsz, "%Y-%m-%d", &g);
}
void ueng_build_timestamp_utc(char* out, size_t outsz) {
  time_t now = time(NULL);
  struct tm g;
#if defined(_WIN32)
  gmtime_s(&g, &now);
#else
  gmtime_r(&now, &g);
#endif
  strftime(out, outsz, "%Y-%m-%dT%H-%M-%SZ", &g);
}

