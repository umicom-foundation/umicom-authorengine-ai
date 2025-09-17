/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (uaengine)
 * Project: Core CLI + Library-Ready Refactor
 * Created by: Umicom Foundation (https://umicom.foundation/)
 * Author: Sammy Hegab + contributors
 * 
 * PURPOSE
 *   This file is part of a refactor that breaks the monolithic main.c into
 *   small, loosely-coupled modules. The goal is to keep the CLI working today
 *   while preparing for a reusable library and a GTK4 IDE (Umicom Studio).
 *
 * LICENSE
 *   SPDX-License-Identifier: MIT
 *---------------------------------------------------------------------------*/
#ifndef UENG_FS_H
#define UENG_FS_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Recursively walk abs_dir; push relative paths (forward slashes) to 'out'.
   rel_dir may be NULL to start at top-level. */
int ingest_walk(const char* abs_dir, const char* rel_dir, StrList* out);

#ifdef __cplusplus
}
#endif

#endif /* UENG_FS_H */
