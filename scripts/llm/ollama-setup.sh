#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# PURPOSE: Project helper script.
#
# Created by: Umicom Foundation (https://umicom.foundation/)
# Developer: Sammy Hegab
# Date: 24-09-2025
# License: MIT
# -----------------------------------------------------------------------------
# NOTE:
# Every line is commented so even a beginner can follow.
# -----------------------------------------------------------------------------

set -euo pipefail

OS="$(uname -s || true)"

if [ "$OS" = "Darwin" ]; then
  echo "Downloading Ollama for macOS to /tmp..."
  curl -fsSL https://ollama.com/download/Ollama-darwin.zip -o /tmp/ollama.zip
  echo "Open /tmp/ollama.zip to install, then run:"
  echo "  ollama serve"
  echo "  ollama pull mistral"
  exit 0
fi

# Linux official installer script
curl -fsSL https://ollama.com/install.sh | sh

# Start server if not running
if ! pgrep -x "ollama" >/dev/null 2>&1; then
  nohup ollama serve >/tmp/ollama.log 2>&1 &
  echo "Ollama server started (log: /tmp/ollama.log)"
fi

# Pull a couple of models
ollama pull mistral
ollama pull llama3
echo "Run:  ollama run mistral"
