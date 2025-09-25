<#-----------------------------------------------------------------------------
 Umicom AuthorEngine AI (uaengine)
 File: scripts/llm/llamacpp-build.ps1
 PURPOSE: Clone/update ggml-org/llama.cpp and build the server target.
 
 Created by: Umicom Foundation (https://umicom.foundation/)
 Author: Sammy Hegab
 Date: 24-09-2025
 License: MIT
-----------------------------------------------------------------------------#>
$ErrorActionPreference = "Stop"
$repoUrl = "https://github.com/ggml-org/llama.cpp.git"
$dest = ".cache/llama.cpp"

Write-Host "[info] Preparing llama.cpp in $dest"
$parent = Split-Path $dest -Parent
if ($parent -and -not (Test-Path $parent)) { New-Item -ItemType Directory -Path $parent | Out-Null }

if (Test-Path (Join-Path $dest ".git")) {
  git -C $dest fetch --depth=1 origin master
  git -C $dest reset --hard origin/master
} else {
  git clone --depth=1 $repoUrl $dest
}

Write-Host "[info] Building server (CPU-only)"
try {
  if (-not (Test-Path (Join-Path $dest "build"))) { New-Item -ItemType Directory -Path (Join-Path $dest "build") | Out-Null }
  pushd (Join-Path $dest "build") | Out-Null
  cmake -S .. -B . -DLLAMA_METAL=OFF -DLLAMA_CUBLAS=OFF -DLLAMA_VULKAN=OFF -DCMAKE_BUILD_TYPE=Release
  cmake --build . --config Release --target server -j
  popd | Out-Null
  Write-Host "[ok] Built llama.cpp server via CMake."
} catch {
  Write-Warning "CMake build failed; falling back to 'make' (requires MSYS2/MinGW)."
  make -C $dest server
}

Write-Host "[hint] Start with: set MODEL=C:\path\to\model.gguf ; scripts\llm\llamacpp-start.ps1"
