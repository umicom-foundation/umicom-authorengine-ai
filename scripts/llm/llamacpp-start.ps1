<#-----------------------------------------------------------------------------
 Umicom AuthorEngine AI (uaengine)
 File: scripts/llm/llamacpp-start.ps1
 PURPOSE: Start llama.cpp HTTP server bound to 127.0.0.1:11434
 
 Created by: Umicom Foundation (https://umicom.foundation/)
 Author: Sammy Hegab
 Date: 24-09-2025
 License: MIT
-----------------------------------------------------------------------------#>
$ErrorActionPreference = "Stop"

if (-not $env:UENG_LLM_ENDPOINT) { $env:UENG_LLM_ENDPOINT = "http://127.0.0.1:11434" }
if (-not $env:MODEL) { Write-Error "MODEL env var not set (path to .gguf)"; exit 1 }

$bin = ".cache/llama.cpp/server"
if (-not (Test-Path $bin)) { Write-Error "Not found: $bin. Run scripts\llm\llamacpp-build.ps1 first."; exit 1 }

$host = "127.0.0.1"; $port = 11434
Write-Host "[info] Starting llama.cpp server on $host:$port with model: $($env:MODEL)"
Start-Process -FilePath $bin -ArgumentList "-m `"$($env:MODEL)`" -c 4096 --host $host --port $port" -WindowStyle Hidden

$ok = $false
1..30 | ForEach-Object {
  try { Invoke-WebRequest -Uri $env:UENG_LLM_ENDPOINT -TimeoutSec 1 -UseBasicParsing | Out-Null; $ok=$true; break } catch { Start-Sleep -Milliseconds 500 }
}
if ($ok) { Write-Host "[ok] Server ready at $($env:UENG_LLM_ENDPOINT)" } else { Write-Warning "Server not yet reporting ready." }
