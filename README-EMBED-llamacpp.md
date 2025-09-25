# Phase 2: Embed llama.cpp inside AuthorEngine

This scaffold adds a **stable wrapper** API and CMake glue so we can link
llama.cpp **in-process**. Nothing breaks if llama.cpp is absent: we compile
portable stubs by default. Turn on embedding with:

```bash
cmake -S . -B build -DUENG_WITH_LLAMA_EMBED=ON
cmake --build build -j
```

## Files in this bundle

- `include/ueng/llm.h` — tiny public API for in-process LLM.
- `src/llm_llama.c` — wrapper (real implementation or friendly stubs).
- `cmake/UENGLlama.cmake` — CMake logic to vendor and link llama.cpp.
- This doc — how to wire it.

## Wire into your build

1) **Place files** into your repo at the same relative locations.

2) In your top-level `CMakeLists.txt`, include the module **after** you define
   the `uaengine` target (or adapt if you prefer before):

```cmake
# add_executable(uaengine ... existing sources ... src/llm_llama.c)
target_sources(uaengine PRIVATE src/llm_llama.c)
include(cmake/UENGLlama.cmake)
```

3) Vendor upstream llama.cpp at `third_party/llama.cpp/` (git submodule or copy).

4) Configure with embedding enabled:

```bash
cmake -S . -B build -DUENG_WITH_LLAMA_EMBED=ON
cmake --build build -j
```

5) Call the API from anywhere (example):

```c
#include "ueng/llm.h"
char err[256]; char out[2048];
ueng_llm_ctx* L = ueng_llm_open("/path/model.gguf", 4096, err, sizeof(err));
if (!L) { fprintf(stderr, "LLM open failed: %s\n", err); }
else {
  if (ueng_llm_prompt(L, "Say hello.", out, sizeof(out))==0)
    printf("LLM: %s\n", out);
  ueng_llm_close(L);
}
```

## Notes

- **MIT** license upstream → compatible with our license (keep upstream LICENSE when vendoring).
- The wrapper uses a **very conservative** llama.cpp API surface to minimize churn.
- If the upstream API changes, adjust only `src/llm_llama.c` (rest of code stays stable).
