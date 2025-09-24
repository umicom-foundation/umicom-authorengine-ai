# PR: Dev Experience — Bootstrap, macOS notarisation, Docker build, Local LLM

## Summary
- One-command **bootstrap** scripts (Windows + Linux/macOS).
- **macOS notarisation** docs for signed & trusted binaries.
- Reproducible **Docker** build image & helper.
- Optional **local LLM** setup (Ollama) + LM Studio notes.

## Changes
- `scripts/bootstrap/bootstrap.ps1` and `scripts/bootstrap/bootstrap.sh`
- `docs/release/macos-notarization.md`
- `docker/ubuntu.Dockerfile`, `docker/build-in-docker.sh`
- `scripts/llm/ollama-setup.ps1`, `scripts/llm/ollama-setup.sh`
- `docs/llm/lm-studio-notes.md`

## Why
Faster onboarding, consistent builds, simple local LLM dev, and cleaner releases.

## Checklist
- [ ] Builds locally with CMake presets.
- [ ] Docker image builds & compiles uaengine.
- [ ] CI green on Linux/macOS/Windows.
- [ ] Docs verified; no artefacts committed.
