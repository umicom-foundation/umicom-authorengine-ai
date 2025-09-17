#include "ueng/common.h"
#include "ueng/fs.h"

/* write full file */
int ueng_write_file(const char* path, const char* content) {
  FILE* f = ueng_fopen(path, "wb");
  if (!f) return -1;
  if (content && *content) fputs(content, f);
  fclose(f);
  return 0;
}

/* write only if not present */
int ueng_write_text_if_absent(const char* path, const char* content) {
  if (ueng_file_exists(path)) return 0;
  return ueng_write_file(path, content ? content : "");
}

int ueng_append_file(FILE* dst, const char* src_path) {
  FILE* src = ueng_fopen(src_path, "rb");
  if (!src) return -1;
  char buf[64*1024];
  size_t rd;
  while ((rd = fread(buf, 1, sizeof(buf), src)) > 0) {
    size_t wr = fwrite(buf, 1, rd, dst);
    if (wr != rd) { fclose(src); return -1; }
  }
  fclose(src);
  return 0;
}

int ueng_write_gitkeep(const char* dir) {
  char path[1024];
  snprintf(path, sizeof(path), "%s%c.gitkeep", dir, PATH_SEP);
  return ueng_write_text_if_absent(path, "");
}

/* ensure parent directory exists */
int ueng_ensure_parent_dir(const char* filepath) {
  const char* last_sep = NULL;
  for (const char* p = filepath; *p; ++p) if (*p == PATH_SEP) last_sep = p;
  if (!last_sep) return 0;
  size_t n = (size_t)(last_sep - filepath);
  char* dir = (char*)malloc(n + 1);
  if (!dir) return -1;
  memcpy(dir, filepath, n);
  dir[n] = '\0';
  int rc = ueng_mkpath(dir);
  free(dir);
  return rc;
}

/* copy (binary) with parents */
int ueng_copy_file_binary(const char* src, const char* dst) {
  FILE* in = ueng_fopen(src, "rb");
  if (!in) return -1;
  if (ueng_ensure_parent_dir(dst) != 0) { fclose(in); return -1; }
  FILE* out = ueng_fopen(dst, "wb");
  if (!out) { fclose(in); return -1; }

  char buf[64*1024];
  size_t rd;
  while ((rd = fread(buf, 1, sizeof(buf), in)) > 0) {
    size_t wr = fwrite(buf, 1, rd, out);
    if (wr != rd) { fclose(in); fclose(out); return -1; }
  }
  fclose(in);
  fclose(out);
  return 0;
}

