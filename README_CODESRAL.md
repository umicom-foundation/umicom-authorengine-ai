# UAEngine — Mistral Backend Additions

This folder contains **drop-in** files to enable a Mistral (Codestral) backend for UAEngine.

## Files added

- `src/llm_mistral.c` — libcurl-based Chat Completions client implementing the UAEngine LLM façade.
- `include/ueng/llm_mistral.h` — minimal header for the Mistral context.
- `patches/CMakeLists_add_mistral.diff` — add the source file to your build.
- `patches/ueng_config_add_env.diff` — optional: add `MISTRAL_API_KEY` and `UENG_MISTRAL_*` keys to config loader.

## Build

Ensure **libcurl** is available. Example with CMake + pkg-config:

```
find_package(CURL REQUIRED)
target_link_libraries(uaengine PRIVATE CURL::libcurl)
```

Then add `src/llm_mistral.c` to your target.

## Use

Set env and run:
```
export MISTRAL_API_KEY=sk-...
export UENG_MISTRAL_MODEL=mistral-small-latest
# UAEngine code should call ueng_llm_open(model, 0, ...) and ueng_llm_prompt(...).
```

> This sample keeps dependencies minimal and uses naive JSON extraction. Replace with yyjson/cJSON for production.
