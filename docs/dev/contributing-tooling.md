# Contributing (tooling)

- Run the OS-specific setup in `scripts/setup/<os>`.
- Use out-of-tree builds (e.g., `build/`) to keep repo clean.
- Prefer **clang + lld** for faster builds where available.
- If you add dependencies, update the relevant setup script and the docs.
