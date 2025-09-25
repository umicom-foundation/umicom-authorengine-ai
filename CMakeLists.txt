# ------------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine) - Top-level CMake
#
# PURPOSE:
#   Cross-platform build configuration with:
#     - Optional compiler cache (sccache) auto-detected
#     - Sensible warnings and defaults per-platform
#     - Feature flags to enable embedded LLM backends
#     - Windows socket lib linking; standard C17 on all platforms
#
# Created by: Umicom Foundation (https://umicom.foundation/)
# Author: Sammy Hegab
# Date: 25-09-2025
# License: MIT
# ------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.16)

# NOTE: We’re a C project (C17). We may add C++ later if needed.
project(uaengine LANGUAGES C)

# ------------------------------- Defaults -------------------------------------
# Default to Release for single-config generators (e.g., Ninja/Makefiles).
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Enforce C17 on all targets by default.
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Optional: make compile_commands.json for IDEs/lint tools.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------- Optional sccache --------------------------------
# If developer has sccache, enable it automatically. If not found, continue.
find_program(SCCACHE_PROGRAM sccache)
if(SCCACHE_PROGRAM)
  message(STATUS "sccache found: ${SCCACHE_PROGRAM}")
  set(CMAKE_C_COMPILER_LAUNCHER   "${SCCACHE_PROGRAM}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${SCCACHE_PROGRAM}")
else()
  message(STATUS "sccache not found; compiling without compiler cache")
endif()

# ----------------------------- Feature flags ----------------------------------
# Build-time toggles that also gate runtime provider availability.
option(UAENG_ENABLE_OPENAI  "Enable OpenAI HTTP backend in binary"  OFF)
option(UAENG_ENABLE_OLLAMA  "Enable Ollama  HTTP backend in binary" OFF)
option(UAENG_ENABLE_LLAMA   "Enable embedded llama.cpp backend"     OFF)

# Add corresponding preprocessor macros for use in source code.
if(UAENG_ENABLE_OPENAI)
  add_compile_definitions(UAENG_ENABLE_OPENAI=1)
endif()
if(UAENG_ENABLE_OLLAMA)
  add_compile_definitions(UAENG_ENABLE_OLLAMA=1)
endif()
if(UAENG_ENABLE_LLAMA)
  add_compile_definitions(UAENG_ENABLE_LLAMA=1)
endif()

# ------------------------------- Warnings -------------------------------------
if(MSVC)
  add_compile_options(/W4 /permissive- /D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
  # Let the source set the language standard; we already set global C17 above.
endif()

# ----------------------------- Include paths ----------------------------------
# Keep headers in a predictable, public include/ tree.
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ------------------------------- Sources --------------------------------------
# NOTE:
#  - The LLM backends are compiled in unconditionally; each file gates
#    functionality internally with UAENG_ENABLE_* macros so the binary
#    remains linkable even when a provider is disabled.
set(UAENG_SOURCES
  src/common.c
  src/fs.c
  src/serve.c
  src/main.c
  src/llm_llama.c
  src/llm_openai.c
  src/llm_ollama.c
)

add_executable(uaengine ${UAENG_SOURCES})

# ------------------------------- Linking --------------------------------------
if(WIN32)
  # Windows sockets library for the tiny HTTP server.
  target_link_libraries(uaengine ws2_32)
endif()

# On non-Windows platforms we generally don't need extra libs.
# If you add pthreads, ssl, etc., do the usual find_package + link here.

# ------------------------------- Install --------------------------------------
# Simple install rule; makes packaging later easier.
install(TARGETS uaengine
  RUNTIME DESTINATION bin
)

# ------------------------------ Status echo -----------------------------------
message(STATUS "Configuration summary:")
message(STATUS "  Generator           : ${CMAKE_GENERATOR}")
message(STATUS "  Build type          : ${CMAKE_BUILD_TYPE}")
message(STATUS "  OpenAI backend      : ${UAENG_ENABLE_OPENAI}")
message(STATUS "  Ollama backend      : ${UAENG_ENABLE_OLLAMA}")
message(STATUS "  llama.cpp backend   : ${UAENG_ENABLE_LLAMA}")
if(CMAKE_C_COMPILER_LAUNCHER)
  message(STATUS "  C compiler launcher : ${CMAKE_C_COMPILER_LAUNCHER}")
endif()
