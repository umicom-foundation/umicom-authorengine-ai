# LLM Integrations (OpenAI & Ollama)

## Build flags

- `-DUAENG_ENABLE_OPENAI=ON`  — include stub OpenAI provider
- `-DUAENG_ENABLE_OLLAMA=ON`  — include stub Ollama provider

Then in runtime:

```
export UENG_LLM_PROVIDER=openai   # or ollama or llama
uaengine llm-selftest gpt-4o-mini
```

These providers are *stubs* (no HTTP yet) so core remains dependency‑free.
