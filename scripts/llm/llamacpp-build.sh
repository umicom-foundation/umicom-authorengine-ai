#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# File: scripts/llm/llamacpp-build.sh
# PURPOSE: Clone/update ggml-org/llama.cpp and build the server target.
#
# Created by: Umicom Foundation (https://umicom.foundation/)
# Author: Sammy Hegab
# Date: 24-09-2025
# License: MIT
# -----------------------------------------------------------------------------
set -euo pipefail

repo_url="https://github.com/ggml-org/llama.cpp.git"
dest=".cache/llama.cpp"

echo "[info] Preparing llama.cpp in $dest"
mkdir -p "$(dirname "$dest")"
if [ -d "$dest/.git" ]; then
  git -C "$dest" fetch --depth=1 origin master
  git -C "$dest" reset --hard origin/master
else
  git clone --depth=1 "$repo_url" "$dest"
fi

echo "[info] Building server (CPU-only)"
make -C "$dest" -j server

echo "[ok] Built: $dest/server"
echo "[hint] Start with: MODEL=/path/to/model.gguf scripts/llm/llamacpp-start.sh"
