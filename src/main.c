/*-----------------------------------------------------------------------------
 * Umicom AuthorEngine AI (ueng) — Minimal CLI in C
 *
 * GOALS:
 *   - Keep it C-first (low-level, fast, portable).
 *   - Provide clear scaffolding for commands we will fill in later.
 *   - Be extremely well-commented for learning and maintainability.
 *
 * SUBCOMMANDS (stubs for now):
 *   ueng init      : Create or inspect a manifest (book.yaml) and prepare workspace.
 *   ueng build     : End-to-end pipeline: ingestor -> outliner -> scribe -> editor -> renderer.
 *   ueng serve     : Preview a static site output on localhost (future).
 *   ueng publish   : (Later) Create/approve a GitHub repo and push artefacts.
 *
 * STYLE:
 *   - Plain ANSI C (C17 baseline). Avoids C++ features by design.
 *   - Minimal dynamic allocation; simple string handling.
 *   - Clear separation of concerns as we grow (each command in its own C file later).
 *---------------------------------------------------------------------------*/

#include <stdio.h>    /* printf, puts, fprintf */
#include <string.h>   /* strcmp */
#include "ueng/version.h"

/* Print a single-line usage help. Keeping it short and discoverable. */
static void usage(void) {
    printf("ueng %s\n", UENG_VERSION);
    printf("Usage: ueng <init|build|serve|publish|--version>\n");
}

/* Each command is a stub now; we print what would happen.
 * Later, these become real functions calling modules under src/.
 */

static int cmd_init(void) {
    /* TODO:
     *  - Detect or create book.yaml in the current directory.
     *  - Scan dropzone/ and suggest defaults (title, author, language).
     *  - Create workspace/ and templates/ if missing.
     */
    puts("[init] Would scaffold manifest (book.yaml) and prepare workspace (TODO).");
    return 0;
}

static int cmd_build(void) {
    /* TODO:
     *  - Ingest text (MD/TXT/PDF), extract images/EXIF.
     *  - Resolve ```ai ... ``` fenced blocks using local LLM/OpenAI when available.
     *  - Outline → Draft → Edit → Render (PDF/DOCX/EPUB/HTML/MD).
     *  - Write outputs under outputs/<slug>/YYYY-mm-ddTHH-MMZ/.
     */
    puts("[build] Would run pipeline: ingestor -> outliner -> scribe -> editor -> renderer (TODO).");
    return 0;
}

static int cmd_serve(void) {
    /* TODO:
     *  - Start a tiny HTTP server (C) to serve outputs/<slug>/site/ for preview.
     *  - Consider a no-dependency approach (sockets) for portability.
     */
    puts("[serve] Would start preview server for static site (TODO).");
    return 0;
}

static int cmd_publish(void) {
    /* TODO:
     *  - Implement Mode 1/2/3 publishing:
     *      1) user-owned repo (token),
     *      2) Umicom-approved imprint (PR to registry -> GH Action),
     *      3) direct org creation (GitHub App, internal only).
     *  - Short term: shell-out to `gh` CLI; long term: call GitHub REST from C (libcurl).
     */
    puts("[publish] Would create or request a repo and push artefacts (TODO).");
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage();
        return 0;
    }

    /* NOTE:
     * Using strcmp is fine here because commands are short and fixed.
     * If you ever parse lots of flags, consider a tiny getopt clone (portable).
     */
    const char *cmd = argv[1];

    if (strcmp(cmd, "init") == 0) {
        return cmd_init();
    } else if (strcmp(cmd, "build") == 0) {
        return cmd_build();
    } else if (strcmp(cmd, "serve") == 0) {
        return cmd_serve();
    } else if (strcmp(cmd, "publish") == 0) {
        return cmd_publish();
    } else if ((strcmp(cmd, "-v") == 0) || (strcmp(cmd, "--version") == 0)) {
        puts(UENG_VERSION);
        return 0;
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        usage();
        return 1;
    }
}
