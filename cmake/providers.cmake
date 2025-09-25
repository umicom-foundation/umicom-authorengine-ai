
# cmake/providers.cmake — optional LLM providers
option(UAENG_ENABLE_OPENAI "Enable OpenAI provider" OFF)
option(UAENG_ENABLE_OLLAMA "Enable Ollama provider" OFF)

# Core facade + default dispatcher (always compiled)
target_sources(uaengine PRIVATE
  src/llm_llama.c
)

# Optional providers
if (UAENG_ENABLE_OPENAI)
  target_compile_definitions(uaengine PRIVATE UAENG_ENABLE_OPENAI=1)
  target_sources(uaengine PRIVATE
    src/providers/llm_openai.c
  )
endif()

if (UAENG_ENABLE_OLLAMA)
  target_compile_definitions(uaengine PRIVATE UAENG_ENABLE_OLLAMA=1)
  target_sources(uaengine PRIVATE src/providers/llm_ollama.c)
endif()
