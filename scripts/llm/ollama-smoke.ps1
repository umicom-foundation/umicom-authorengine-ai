$ErrorActionPreference = "Stop"
if (-not $env:UENG_LLM_ENDPOINT) { $env:UENG_LLM_ENDPOINT = "http://127.0.0.1:11434" }
Write-Host "[info] GET $($env:UENG_LLM_ENDPOINT)/api/tags"
Invoke-WebRequest -Uri "$($env:UENG_LLM_ENDPOINT)/api/tags" -TimeoutSec 5 -UseBasicParsing | Out-Null
Write-Host "[ok] reachable"
Write-Host "[info] POST /api/chat (tiny message)"
$body = @{ model="llama3.2:1b"; stream=$false; messages=@(@{role="user"; content="Say OK."}) } | ConvertTo-Json
Invoke-WebRequest -Uri "$($env:UENG_LLM_ENDPOINT)/api/chat" -Method Post -ContentType "application/json" -Body $body -UseBasicParsing | Out-Null
Write-Host "[ok] chat responded"
