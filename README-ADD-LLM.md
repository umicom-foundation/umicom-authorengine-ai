# LLM embedding (llama.cpp) – drop-in bundle

This bundle fixes your current linker errors:

```
unresolved external symbol ueng_llm_open
unresolved external symbol ueng_llm_prompt
unresolved external symbol ueng_llm_close
```

## What’s included

- `include/ueng/llm.h` – public C API
- `src/llm_llama.c` – implementation
  - Compiles **always**.
  - When not embedding llama.cpp, it builds **stub** functions (returning a clear runtime message).
  - When you later vendor `llama.cpp` and configure CMake with `-DUENG_WITH_LLAMA_EMBED=ON` and `-DHAVE_LLAMA_H=ON`, the real backend is used.

## How to wire into your build

1. Place these files into your tree:
   - `include/ueng/llm.h` → `<repo_root>/include/ueng/llm.h`
   - `src/llm_llama.c`    → `<repo_root>/src/llm_llama.c`

2. Update **CMakeLists.txt** to compile the source and expose the headers:

```cmake
# Ensure our public headers are visible
include_directories(${CMAKE_SOURCE_DIR}/include)

# Add the LLM implementation to the uaengine target
# (Replace 'uaengine' with your actual target name if different)
target_sources(uaengine PRIVATE src/llm_llama.c)
```

> Tip: If you use a variable for sources, just append `src/llm_llama.c` there.

3. Rebuild:

```powershell
cmake -S . -B build
cmake --build build -j
```

4. Test the CLI command you added earlier (it links now because symbols exist):

```powershell
.\build\Debug\uaengine.exe llm-selftest C:\path\to\model.gguf
```

Without embedding llama.cpp yet, you’ll get a friendly stub message. Later, when you vendor `llama.cpp` and set the CMake flags, the same binary will run real inference.

---

© Umicom Foundation – Created by Sammy Hegab (24-09-2025)
