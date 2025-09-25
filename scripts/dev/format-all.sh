#!/usr/bin/env bash
set -euo pipefail
git ls-files '*.c' '*.h' | xargs -r clang-format -i
