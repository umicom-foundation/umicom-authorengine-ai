# Run helpers

Use `scripts/run-uaengine.ps1` (Windows PowerShell) or `scripts/run-uaengine.sh` (Linux/macOS) to locate a built binary across common build folders and pass through arguments.

Examples:
- `powershell -ExecutionPolicy Bypass -File scripts/run-uaengine.ps1 -- --version`
- `bash scripts/run-uaengine.sh -- llm-selftest gpt-4o-mini`
