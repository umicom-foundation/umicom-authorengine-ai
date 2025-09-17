#ifndef UENG_FS_H
#define UENG_FS_H

#include <stdio.h>
#include <stddef.h>

/* write whole-text file (UTF-8) */
int  ueng_write_file(const char* path, const char* content);
/* write if absent (useful for seeds) */
int  ueng_write_text_if_absent(const char* path, const char* content);
/* append a whole file into an open stream */
int  ueng_append_file(FILE* dst, const char* src_path);
/* create .gitkeep in a directory (if not present) */
int  ueng_write_gitkeep(const char* dir);

/* ensure parent directory of "filepath" exists */
int  ueng_ensure_parent_dir(const char* filepath);
/* copy file (binary) creating dirs as needed */
int  ueng_copy_file_binary(const char* src, const char* dst);

#endif /* UENG_FS_H */

