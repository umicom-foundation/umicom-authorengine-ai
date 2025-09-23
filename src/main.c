/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine) — CLI front-end (main)
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab
 * Date: 15-09-2025
 * PURPOSE
 *   Thin command dispatcher that delegates all heavy work to modules:
 *     - common.{h,c}  : platform shims, file ops, strings, time, exec, etc.
 *     - fs.{h,c}      : ingest/normalize/build assets and draft packing
 *     - serve.c       : tiny static HTTP server (serve_run)
 *---------------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>

#include "ueng/version.h"
#include "ueng/common.h"
#include "ueng/fs.h"
#include "ueng/serve.h"

/*---- Forward declarations (local only) ------------------------------------*/
static int  cmd_init(void);
static int  cmd_ingest(void);
static int  cmd_build(void);
static int  cmd_export(void);
static int  cmd_serve(int argc, char** argv);
static int  cmd_publish(void);
static void usage(void);
static void usage_cmd(const char* cmd);
static void usage_init(void);
static void usage_ingest(void);
static void usage_build(void);
static void usage_export(void);
static void usage_serve(void);
static void usage_publish(void);

/* local helpers kept here for convenience */
static int  seed_book_yaml(void);
static int  tiny_yaml_get(const char* filename, const char* key, char* out, size_t outsz);
static int  tiny_yaml_get_bool(const char* filename, const char* key, int* out);
static int  write_outline_md(const char* title, const char* author,
                             const char* dropzone_rel, const StrList* files);

/*----------------------------------------------------------------------------*/
/* starter manifest */
static int seed_book_yaml(void) {
  const char *yaml =
    "# Umicom AuthorEngine AI — Book manifest (starter)\n"
    "title: \"My New Book\"\n"
    "subtitle: \"Learning by Building\"\n"
    "author: \"Your Name\"\n"
    "language: \"en-GB\"\n"
    "publisher: \"\"\n"
    "copyright_year: \"2025\"\n"
    "description: \"Short paragraph describing the book.\"\n"
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

/* tiny yaml (single-line key: value) */
static int tiny_yaml_get(const char* filename, const char* key, char* out, size_t outsz) {
  FILE* f = ueng_fopen(filename, "rb");
  if (!f) return -1;
  char line[1024];
  size_t keylen = strlen(key);
  int found = 0;

  while (fgets(line, (int)sizeof(line), f)) {
    char* s = ltrim(line);
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
  char buf[64] = {0};
  int rc = tiny_yaml_get(filename, key, buf, sizeof(buf));
  if (rc != 0) return rc;
  for (char* p = buf; *p; ++p) *p = (char)tolower((unsigned char)*p);
  if (!strcmp(buf, "true") || !strcmp(buf, "yes") || !strcmp(buf, "on") || !strcmp(buf, "1")) { *out = 1; return 0; }
  if (!strcmp(buf, "false")|| !strcmp(buf, "no")  || !strcmp(buf, "off")|| !strcmp(buf, "0")) { *out = 0; return 0; }
  *out = (buf[0] != '\0'); return 0;
}

/* outline writer */
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
    "# Draft Outline - %s\n\n"
    "_Author:_ **%s**  \n"
    "_Date:_ **%s**  \n"
    "_Sources scanned:_ `%s`\n\n"
    "## Sources (recursive)\n",
    title ? title : "Untitled",
    author ? author : "Unknown",
    day, dropzone_rel ? dropzone_rel : "dropzone");
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

/*----------------------------- Commands ------------------------------------*/
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

static int cmd_ingest(void) {
  char title[256]={0}, author[256]={0}, drop[256]={0};
  if (tiny_yaml_get("book.yaml","title", title,sizeof(title))  != 0) snprintf(title, sizeof(title),  "%s","Untitled");
  if (tiny_yaml_get("book.yaml","author",author,sizeof(author))!= 0) snprintf(author,sizeof(author), "%s","Unknown");
  if (tiny_yaml_get("book.yaml","dropzone",drop,sizeof(drop)) != 0) snprintf(drop, sizeof(drop),    "%s","dropzone");

  if (!file_exists(drop)) {
    fprintf(stderr, "[ingest] ERROR: dropzone path not found: %s\n", drop);
    return 1;
  }
  StrList files; sl_init(&files);
  int rc = ingest_walk(drop, NULL, &files);
  if (rc != 0) { fprintf(stderr, "[ingest] ERROR: directory walk failed\n"); sl_free(&files); return 1; }

  if (files.count > 1) qsort(files.items, files.count, sizeof(char*), qsort_nat_ci_cmp);

  rc = write_outline_md(title, author, drop, &files);
  sl_free(&files);
  if (rc != 0) { fprintf(stderr, "[ingest] ERROR: failed to write outline\n"); return 1; }

  puts("[ingest] complete.");
  return 0;
}

static int cmd_build(void) {
  char title[256]={0}, author[256]={0}; int rc;
  rc = tiny_yaml_get("book.yaml","title", title,sizeof(title));
  if (rc == -1) { fprintf(stderr,"[build] ERROR: cannot open book.yaml (run `uaengine init`)\n"); return 1; }
  if (rc ==  1) { snprintf(title,sizeof(title),"%s","Untitled"); }
  rc = tiny_yaml_get("book.yaml","author",author,sizeof(author));
  if (rc == -1) { fprintf(stderr,"[build] ERROR: cannot open book.yaml\n"); return 1; }
  if (rc ==  1) { snprintf(author,sizeof(author),"%s","Unknown"); }

  int ingest_first = 0;
  if (tiny_yaml_get_bool("book.yaml", "ingest_on_build", &ingest_first) == 0 && ingest_first) {
    puts("[build] ingest_on_build: true — running ingest...");
    if (cmd_ingest() != 0) fprintf(stderr, "[build] WARN: ingest failed; continuing.\n");
  }
  int norm = 0;
  char drop[256]={0};
  if (tiny_yaml_get("book.yaml","dropzone",drop,sizeof(drop)) != 0) snprintf(drop,sizeof(drop),"%s","dropzone");
  if (tiny_yaml_get_bool("book.yaml","normalize_chapters_on_build",&norm) == 0 && norm) {
    printf("[build] normalize_chapters_on_build: true — mirroring from %s\n", drop);
    if (normalize_chapters(drop) != 0) fprintf(stderr,"[build] WARN: chapter normalization failed; continuing.\n");
  }

  (void)generate_toc_md(title);
  (void)generate_frontmatter_md(title, author);
  (void)generate_acknowledgements_md(author);

  char slug[256]; slugify(title, slug, sizeof(slug));
  char day[32];   build_date_utc(day, sizeof(day));
  char stamp[64]; build_timestamp_utc(stamp, sizeof(stamp));

  char root[512];
  snprintf(root,sizeof(root),"outputs%c%s%c%s",PATH_SEP,slug,PATH_SEP,day);

  if (file_exists(root)) {
    printf("[build] cleaning existing: %s\n", root);
    if (clean_dir(root) != 0) fprintf(stderr,"[build] WARN: could not fully clean %s\n", root);
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

  /* ensure default CSS available for serve */
  {
    char html_dir[640]; snprintf(html_dir,sizeof(html_dir),"%s%c%s",root,PATH_SEP,"html");
    char rel_css_tmp[16];
    (void)copy_theme_into_html_dir(html_dir, rel_css_tmp, sizeof(rel_css_tmp));
  }

  /* cover */
  char ws_cover[640]; snprintf(ws_cover, sizeof(ws_cover), "workspace%ccover.svg", PATH_SEP);
  if (!file_exists(ws_cover)) {
    char ws_chap_cover[640]; snprintf(ws_chap_cover, sizeof(ws_chap_cover), "workspace%cchapters%ccover.svg", PATH_SEP, PATH_SEP);
    if (file_exists(ws_chap_cover)) (void)copy_file_binary(ws_chap_cover, ws_cover);
    else (void)generate_cover_svg(title, author, slug);
  }
  (void)generate_frontcover_md(title, author, slug);

  int has_cover = 0;
  char cover_dst_archive[640]; snprintf(cover_dst_archive, sizeof(cover_dst_archive), "%s%ccover%ccover.svg", root, PATH_SEP, PATH_SEP);
  char cover_dst_site[640];    snprintf(cover_dst_site,    sizeof(cover_dst_site),    "%s%csite%ccover.svg",  root, PATH_SEP, PATH_SEP);
  if (file_exists(ws_cover)) {
    if (copy_file_binary(ws_cover, cover_dst_archive) == 0) {
      has_cover = file_exists(cover_dst_archive);
      if (has_cover) printf("[cover] copied (archive): %s\n", cover_dst_archive);
    }
    if (copy_file_binary(ws_cover, cover_dst_site) == 0) {
      if (file_exists(cover_dst_site)) { printf("[cover] copied (site): %s\n", cover_dst_site); has_cover = 1; }
    }
  }

  char info_path[640]; snprintf(info_path,sizeof(info_path),"%s%cBUILDINFO.txt",root,PATH_SEP);
  char info[1024];
  int n = snprintf(info,sizeof(info),
      "Title:  %s\nAuthor: %s\nSlug:   %s\nDate:   %s\nStamp:  %s\n",
      title,author,slug,day,stamp);
  if (n<0 || n>=(int)sizeof(info)) fprintf(stderr,"[build] WARN: info truncated\n");
  if (write_file(info_path, info)!=0) { fprintf(stderr,"[build] ERROR: cannot write BUILDINFO.txt\n"); return 1; }

  int has_draft = 0;
  if (pack_book_draft(title, root, &has_draft) != 0) fprintf(stderr, "[build] WARN: failed to create book-draft.md\n");

  char site_dir[640]; snprintf(site_dir,sizeof(site_dir),"%s%c%s",root,PATH_SEP,"site");
  if (write_site_index(site_dir, title, author, slug, stamp, has_cover, has_draft) != 0) {
    fprintf(stderr,"[build] WARN: could not write site/index.html\n");
  }

  printf("[build] ok: %s\n", root);
  puts("[build] outputs will be overwritten on subsequent builds for the same date.");
  return 0;
}

static int cmd_export(void) {
  /* read title/slug/day */
  char title[256]={0}, author[256]={0};
  if (tiny_yaml_get("book.yaml","title", title,sizeof(title))  != 0) snprintf(title,sizeof(title),  "%s","Untitled");
  if (tiny_yaml_get("book.yaml","author",author,sizeof(author))!= 0) snprintf(author,sizeof(author), "%s","Unknown");
  char slug[256]; slugify(title, slug, sizeof(slug));
  char day[32]; build_date_utc(day, sizeof(day));

  /* ensure build-draft exists */
  if (!file_exists("workspace/book-draft.md")) {
    fprintf(stderr, "[export] workspace/book-draft.md not found. Run `uaengine build` first.\n");
    return 1;
  }

  /* paths */
  char root[512]; snprintf(root,sizeof(root),"outputs%c%s%c%s",PATH_SEP,slug,PATH_SEP,day);
  char html_dir[640]; snprintf(html_dir,sizeof(html_dir), "%s%chtml", root, PATH_SEP);
  char pdf_dir[640];  snprintf(pdf_dir, sizeof(pdf_dir),  "%s%cpdf",  root, PATH_SEP);
  (void)mkpath(html_dir); (void)mkpath(pdf_dir);

  /* copy theme -> html/style.css */
  char rel_css[64]; rel_css[0]='\0';
  if (copy_theme_into_html_dir(html_dir, rel_css, sizeof(rel_css)) != 0) {
    fprintf(stderr, "[export] WARN: could not copy theme; continuing.\n");
  }

  /* pandoc -> HTML */
  char out_html[768]; snprintf(out_html, sizeof(out_html), "%s%cbook.html", html_dir, PATH_SEP);
  char cmd1[2048];
  if (rel_css[0]) {
    snprintf(cmd1, sizeof(cmd1),
      "pandoc \"workspace%cbook-draft.md\" -f markdown -t html5 -s --toc --metadata title=\"%s\" -M author=\"%s\" --resource-path=\".;dropzone;workspace\" -c \"%s\" -o \"%s\"",
      PATH_SEP, title, author, rel_css, out_html);
  } else {
    snprintf(cmd1, sizeof(cmd1),
      "pandoc \"workspace%cbook-draft.md\" -f markdown -t html5 -s --toc --metadata title=\"%s\" -M author=\"%s\" --resource-path=\".;dropzone;workspace\" -o \"%s\"",
      PATH_SEP, title, author, out_html);
  }
  if (exec_cmd(cmd1) != 0) {
    fprintf(stderr, "[export] ERROR: pandoc HTML failed.\n");
    return 1;
  }

  /* headless HTML->PDF */
  char pdf_path[768]; snprintf(pdf_path, sizeof(pdf_path), "%s%cbook.pdf", pdf_dir, PATH_SEP);
#ifdef _WIN32
  const char* candidates[] = {
    "C:\\\\Program Files (x86)\\\\Microsoft\\\\Edge\\\\Application\\\\msedge.exe",
    "C:\\\\Program Files\\\\Microsoft\\\\Edge\\\\Application\\\\msedge.exe",
    "C:\\\\Program Files\\\\Google\\\\Chrome\\\\Application\\\\chrome.exe",
    "C:\\\\Program Files (x86)\\\\Google\\\\Chrome\\\\Application\\\\chrome.exe",
    NULL
  };
  char browser[512] = {0};
  for (int i=0; candidates[i]; ++i) {
    if (file_exists(candidates[i])) { snprintf(browser, sizeof(browser), "%s", candidates[i]); break; }
  }
  if (!browser[0]) {
    fprintf(stderr, "[export] WARN: Edge/Chrome not found; skipping PDF.\n");
    printf("[export] HTML: %s\n", out_html);
    return 0;
  }

  char abs_html[1024]; if (path_abs(out_html, abs_html, sizeof(abs_html)) != 0) snprintf(abs_html, sizeof(abs_html), "%s", out_html);
  char file_url[1200]; path_to_file_url(abs_html, file_url, sizeof(file_url));

  char cmd2[2048];
  /* wrap in cmd /C to handle spaces in path robustly */
  snprintf(cmd2, sizeof(cmd2),
    "cmd /C \"\"%s\" --headless=new --disable-gpu --print-to-pdf=\"%s\" --print-to-pdf-no-header --no-margins --run-all-compositor-stages-before-draw --virtual-time-budget=10000 \"%s\"\"",
    browser, pdf_path, file_url);

  if (exec_cmd(cmd2) != 0) {
    fprintf(stderr, "[export] WARN: headless PDF failed (browser).\n");
  } else {
    printf("[export] PDF: %s\n", pdf_path);
  }
#else
  /* Non-Windows: try wkhtmltopdf as a simple default */
  const char* wk = "wkhtmltopdf";
  char cmd2[2048];
  snprintf(cmd2, sizeof(cmd2), "%s \"%s\" \"%s\"", wk, out_html, pdf_path);
  if (exec_cmd(cmd2) != 0) fprintf(stderr, "[export] WARN: wkhtmltopdf failed or not installed.\n");
#endif

  printf("[export] HTML: %s\n", out_html);
  return 0;
}

static int cmd_serve(int argc, char** argv) {
  const char* host = "127.0.0.1";
  int port = 8080;
  if (argc >= 3) host = argv[2];
  if (argc >= 4) { port = atoi(argv[3]); if (port<=0) port=8080; }

  /* Optional override via env var (serve an older build etc.) */
  const char* override = getenv("UENG_SITE_ROOT");
  char site_root[1024];

  if (override && *override) {
    snprintf(site_root, sizeof(site_root), "%s", override);
  } else {
    /* title -> slug; date -> YYYY-MM-DD; site folder */
    char title[256] = {0};
    if (tiny_yaml_get("book.yaml","title", title, sizeof(title)) != 0) {
      snprintf(title, sizeof(title), "%s", "Untitled");
    }
    char slug[256]; slugify(title, slug, sizeof(slug));
    char day[32]; build_date_utc(day, sizeof(day));
    snprintf(site_root, sizeof(site_root), "outputs%c%s%c%s%csite",
             PATH_SEP, slug, PATH_SEP, day, PATH_SEP);
  }

  if (!file_exists(site_root)) {
    fprintf(stderr, "[serve] ERROR: site root not found: %s\n", site_root);
    fprintf(stderr, "[serve] HINT: Run `uaengine build` today or set UENG_SITE_ROOT to a specific path\n");
    return 1;
  }

  return serve_run(site_root, host, port);
}

static int cmd_publish(void) {
  fprintf(stderr, "[publish] not implemented yet.\n");
  return 1;
}

/*--------------------------------- main ------------------------------------*/
int main(int argc, char **argv) {
#ifdef _WIN32
  ueng_console_utf8();
#endif
  if (argc < 2) { usage(); return 0; }

  /* Global flags */
  if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
    usage();
    return 0;
  }
  if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0) {
    puts(UENG_VERSION_STRING);
    return 0;
  }
  if (strcmp(argv[1], "help") == 0) {
    if (argc >= 3) usage_cmd(argv[2]); else usage();
    return 0;
  }

  const char *cmd = argv[1];

  /* Per-command --help passthroughs to print usage then exit 0 */
  if ((strcmp(cmd,"init")==0)   && argc>=3 && strcmp(argv[2],"--help")==0)    { usage_init(); return 0; }
  if ((strcmp(cmd,"ingest")==0) && argc>=3 && strcmp(argv[2],"--help")==0)    { usage_ingest(); return 0; }
  if ((strcmp(cmd,"build")==0)  && argc>=3 && strcmp(argv[2],"--help")==0)    { usage_build(); return 0; }
  if ((strcmp(cmd,"export")==0) && argc>=3 && strcmp(argv[2],"--help")==0)    { usage_export(); return 0; }
  if ((strcmp(cmd,"serve")==0)  && argc>=3 && strcmp(argv[2],"--help")==0)    { usage_serve(); return 0; }
  if ((strcmp(cmd,"publish")==0)&& argc>=3 && strcmp(argv[2],"--help")==0)    { usage_publish(); return 0; }

  if      (strcmp(cmd, "init")    == 0) return cmd_init();
  else if (strcmp(cmd, "ingest")  == 0) return cmd_ingest();
  else if (strcmp(cmd, "build")   == 0) return cmd_build();
  else if (strcmp(cmd, "export")  == 0) return cmd_export();
  else if (strcmp(cmd, "serve")   == 0) return cmd_serve(argc, argv);
  else if (strcmp(cmd, "publish") == 0) return cmd_publish();
  else { fprintf(stderr, "Unknown command: %s\n", cmd); usage(); return 1; }
}



static void usage_init(void){
  puts("Usage: uaengine init\n");
  puts("Initialize a new book project structure (book.yaml, workspace/, dropzone/).");
}
static void usage_ingest(void){
  puts("Usage: uaengine ingest\n");
  puts("Scan ./dropzone and copy Markdown files into workspace/chapters (normalized).");
}
static void usage_build(void){
  puts("Usage: uaengine build\n");
  puts("Concatenate workspace/chapters into workspace/book-draft.md.");
  puts("Respects 'ingest_on_build: true' in book.yaml to run ingest first.");
}
static void usage_export(void){
  puts("Usage: uaengine export\n");
  puts("Create outputs/<slug>/<YYYY-MM-DD>/{html,site} from the current draft.");
}
static void usage_serve(void){
  puts("Usage: uaengine serve [host] [port]\n");
  puts("Serve outputs/<slug>/<date>/site over HTTP (default 127.0.0.1 8080).");
  puts("Env: UENG_SITE_ROOT can point to a specific site folder to serve.");
}
static void usage_publish(void){
  puts("Usage: uaengine publish\n");
  puts("Placeholder command. Not implemented yet.");
}
static void usage_cmd(const char* cmd){
  if(!cmd){ usage(); return; }
  if(strcmp(cmd,"init")==0) { usage_init(); return; }
  if(strcmp(cmd,"ingest")==0){ usage_ingest(); return; }
  if(strcmp(cmd,"build")==0){ usage_build(); return; }
  if(strcmp(cmd,"export")==0){ usage_export(); return; }
  if(strcmp(cmd,"serve")==0){ usage_serve(); return; }
  if(strcmp(cmd,"publish")==0){ usage_publish(); return; }
  usage();
}
static void usage(void) {
  puts("Umicom AuthorEngine AI (uaengine) - Manage your book projects with AI assistance.\n");
  puts("Usage: uaengine <command> [options]\n");
  puts("Commands:");
  puts("  init                 Initialize a new book project structure.");
  puts("  ingest               Ingest and organize content from the dropzone.");
  puts("  build                Build the book draft and prepare outputs.");
  puts("  export               Export the book to HTML and PDF formats.");
  puts("  serve [host] [port]  Serve outputs/<slug>/<date>/site over HTTP (default 127.0.0.1 8080).");
  puts("  publish              Publish the book to a remote server (not implemented).");
  puts("  --version            Show version information.");
  puts("\nRun 'uaengine <command> --help' for command-specific options.");
}
