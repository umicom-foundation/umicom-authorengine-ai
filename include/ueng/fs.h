/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * File: include/ueng/fs.h
 * Purpose: Ingestion, normalization, draft packing and simple generators
 *
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab + contributors
 * License: MIT
 *---------------------------------------------------------------------------*/
#ifndef UENG_FS_H
#define UENG_FS_H

#include <stddef.h>
#include "ueng/common.h"

#ifdef __cplusplus
extern "C" {
#endif

int  ingest_walk(const char* abs_dir, const char* rel_dir, StrList* out);
int  normalize_chapters(const char* dropzone);

int  generate_toc_md(const char* book_title);
int  generate_frontmatter_md(const char* title, const char* author);
int  generate_acknowledgements_md(const char* author);
int  generate_cover_svg(const char* title, const char* author, const char* slug);
int  generate_frontcover_md(const char* title, const char* author, const char* slug);
int  pack_book_draft(const char* title, const char* outputs_root, int* out_has_draft);

int  copy_theme_into_html_dir(const char* html_dir, char* out_rel_css, size_t outsz);
int  write_site_index(const char* site_dir,
                      const char* title,
                      const char* author,
                      const char* slug,
                      const char* stamp,
                      int has_cover,
                      int has_draft);

#ifdef __cplusplus
}
#endif
#endif /* UENG_FS_H */
