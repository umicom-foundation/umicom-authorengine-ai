$ErrorActionPreference = "Stop"
if (-not $env:UENG_LLM_ENDPOINT) { $env:UENG_LLM_ENDPOINT = "http://127.0.0.1:11434" }
Write-Host "[info] Target endpoint: $($env:UENG_LLM_ENDPOINT)"
$ollama = Get-Command ollama -ErrorAction SilentlyContinue
if ($ollama) {
  try {
    Invoke-WebRequest -Uri "$($env:UENG_LLM_ENDPOINT)/api/tags" -TimeoutSec 2 -UseBasicParsing | Out-Null
  } catch {
    Start-Process -FilePath $ollama.Source -ArgumentList "serve" -WindowStyle Hidden
    $ready = $false
    1..30 | ForEach-Object {
      try { Invoke-WebRequest -Uri "$($env:UENG_LLM_ENDPOINT)/api/tags" -TimeoutSec 1 -UseBasicParsing | Out-Null; $ready=$true; break } catch { Start-Sleep -Milliseconds 500 }
    }
  }
  param([string]$Model = "llama3.2:1b")
  $list = (ollama list) -join "`n"
  if ($list -notmatch "^{Model}$") { Write-Host "[info] Pulling $Model"; ollama pull $Model | Write-Host }
  Write-Host "[ok] Endpoint: $($env:UENG_LLM_ENDPOINT)"
} else {
  Write-Warning "'ollama' not found. Use Docker compose (docker/docker-compose.ollama.yaml)."
}
