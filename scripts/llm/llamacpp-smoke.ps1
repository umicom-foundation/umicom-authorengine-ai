$ErrorActionPreference = "Stop"
if (-not $env:UENG_LLM_ENDPOINT) { $env:UENG_LLM_ENDPOINT = "http://127.0.0.1:11434" }
Write-Host "[info] GET $($env:UENG_LLM_ENDPOINT)"
Invoke-WebRequest -Uri $env:UENG_LLM_ENDPOINT -TimeoutSec 5 -UseBasicParsing | Out-Null
Write-Host "[ok] reachable"
