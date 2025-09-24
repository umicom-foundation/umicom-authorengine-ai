### Commit style
- Conventional commits recommended (feat, fix, chore, docs, refactor).
- Keep PRs focused; one logical change per PR.

### Code style
- Run `pre-commit install` once; hooks will run on each commit.
- C/C++ formatted by `clang-format` (see `.clang-format` in repo).

### CI
- All PRs must pass GitHub Actions CI.
- Tagged commits `vX.Y.Z` trigger release builds and upload artefacts.
