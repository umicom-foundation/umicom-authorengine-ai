#-----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# File: CMakeLists.txt
# PURPOSE: Cross‑platform build configuration for the uaengine CLI with
#          optional LLM backends (OpenAI API, Ollama HTTP, llama.cpp local).
#
# Created by: Umicom Foundation (https://umicom.foundation/)
# Author: Sammy Hegab
# Date: 25-09-2025
# License: MIT
#-----------------------------------------------------------------------------
# DESIGN NOTES
#  - Keep the project simple: a single CLI target `uaengine` built from C.
#  - Make backends opt-in or auto-detected, but never required.
#  - Work well with Visual Studio (MSBuild), Ninja, and Xcode generators.
#  - Avoid global includes; prefer target-scoped properties.
#  - On Windows, link ws2_32 for sockets used by the tiny HTTP server.
#  - When a backend is enabled, a clear compile definition is set so
#    source files can compile the correct implementation paths.
#-----------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.20)

# Project in C only (we don't need C++; avoids some CMake generator quirks)
project(uaengine C)

# Options (users can toggle with -D...=ON/OFF).
option(UAENG_ENABLE_OPENAI "Enable OpenAI HTTP backend (requires API key)" ON)
option(UAENG_ENABLE_OLLAMA "Enable Ollama HTTP backend" ON)
option(UAENG_ENABLE_LLAMA  "Enable embedded llama.cpp backend" ON)

# Optional compiler cache (harmless if missing). We *don't* fail if sccache is
# not installed; this is a comfort knob for developer machines and CI.
find_program(SCCACHE_EXECUTABLE sccache)
if(SCCACHE_EXECUTABLE)
  message(STATUS "sccache found: " ${SCCACHE_EXECUTABLE})
  set(CMAKE_C_COMPILER_LAUNCHER   ${SCCACHE_EXECUTABLE} CACHE STRING "" FORCE)
else()
  message(STATUS "sccache not found; compiling without compiler cache")
endif()

# Configure warnings and a sane default language standard for all compilers.
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(MSVC)
  # /W4 = high warnings; /D_CRT_SECURE_NO_WARNINGS quiets MSVC's CRT noise.
  add_compile_options(/W4 /D_CRT_SECURE_NO_WARNINGS)
else()
  add_compile_options(-Wall -Wextra -Wno-unused-parameter)
endif()

# Source layout (keep explicit, easy to read; order isn't important).
set(UAENG_SRC
  src/common.c
  src/fs.c
  src/serve.c
  src/ueng_config.c
  src/llm_llama.c
  src/llm_openai.c
  src/llm_ollama.c
  src/main.c
)

# The main CLI target.
add_executable(uaengine ${UAENG_SRC})

# Public headers live under include/ (and include/ueng). We expose them to the
# compiler, but do not install anything yet (CLI app only for now).
target_include_directories(uaengine PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Platform libraries (Winsock on Windows; pthreads on POSIX via CMake Threads).
if(WIN32)
  target_link_libraries(uaengine PRIVATE ws2_32)
else()
  find_package(Threads REQUIRED)
  target_link_libraries(uaengine PRIVATE Threads::Threads)
endif()

# ----------------------------- LLM Backends ----------------------------------
# We compile all three .c files unconditionally (they are tiny), but guard the
# *implementation* inside with UAENG_ENABLE_* macros. This keeps the linker and
# IDE happy while letting users disable/enable backends without touching sources.
if(UAENG_ENABLE_OPENAI)
  target_compile_definitions(uaengine PRIVATE UAENG_ENABLE_OPENAI=1)
endif()
if(UAENG_ENABLE_OLLAMA)
  target_compile_definitions(uaengine PRIVATE UAENG_ENABLE_OLLAMA=1)
endif()

# llama.cpp integration:
# - Assumes the repository is present as a Git submodule at third_party/llama.cpp
# - We turn off examples/tests/server so the library target 'llama' is produced.
# - If the submodule is missing or CMakeLists.txt isn't there, we proceed
#   without embedding (the llm_llama.c will print a friendly message).
set(LLAMA_SUBDIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/llama.cpp)
if(UAENG_ENABLE_LLAMA AND EXISTS ${LLAMA_SUBDIR}/CMakeLists.txt)
  message(STATUS "llama.cpp submodule found; building embedded backend")
  set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(LLAMA_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
  set(LLAMA_BUILD_SERVER   OFF CACHE BOOL "" FORCE)
  set(LLAMA_STATIC         ON  CACHE BOOL "" FORCE)
  add_subdirectory(${LLAMA_SUBDIR} build-llama EXCLUDE_FROM_ALL)
  target_link_libraries(uaengine PRIVATE llama)
  target_compile_definitions(uaengine PRIVATE UAENG_ENABLE_LLAMA=1)
else()
  if(UAENG_ENABLE_LLAMA)
    message(WARNING "UAENG_ENABLE_LLAMA=ON but third_party/llama.cpp not present; embedded backend will be disabled at compile time.")
  endif()
endif()

# ---------------------------- Build Summary ----------------------------------
message(STATUS "Configuration summary:")
message(STATUS "  Generator           : ${CMAKE_GENERATOR}")
message(STATUS "  Build type          : ${CMAKE_BUILD_TYPE}")
message(STATUS "  OpenAI backend      : ${UAENG_ENABLE_OPENAI}")
message(STATUS "  Ollama backend      : ${UAENG_ENABLE_OLLAMA}")
message(STATUS "  llama.cpp backend   : ${UAENG_ENABLE_LLAMA}")

# On MSVC + Ninja, produce uaengine.exe next to build.ninja for easy launch.
set_target_properties(uaengine PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
