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

# Name the image; version pin keeps compilers stable
IMAGE_NAME="umicom/uaengine-build:ubuntu-24.04"

# Build (cached) image from Dockerfile
docker build -t "$IMAGE_NAME" -f docker/ubuntu.Dockerfile .

# Run container with repo mounted at /workspace
docker run --rm -it   -v "$(pwd)":/workspace   -w /workspace   "$IMAGE_NAME"   bash -lc 'cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ./build/uaengine --help || true'
