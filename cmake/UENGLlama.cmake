# -----------------------------------------------------------------------------
# Umicom AuthorEngine AI (uaengine)
# File: cmake/UENGLlama.cmake
# PURPOSE: Optional embedding of llama.cpp (Phase 2).
#
# Created by: Umicom Foundation (https://umicom.foundation/)
# Author: Sammy Hegab
# Date: 24-09-2025
# License: MIT
# -----------------------------------------------------------------------------

option(UENG_WITH_LLAMA_EMBED "Build with embedded llama.cpp (in-process)" OFF)

# Where the user vendors llama.cpp (recommended):
#   third_party/llama.cpp/   (contains CMakeLists.txt, include/llama.h, etc.)
set(UENG_LLAMA_SRC_DIR "${CMAKE_SOURCE_DIR}/third_party/llama.cpp")

if(UENG_WITH_LLAMA_EMBED)
  if(EXISTS "${UENG_LLAMA_SRC_DIR}/CMakeLists.txt")
    message(STATUS "[ueng] Enabling embedded llama.cpp from: ${UENG_LLAMA_SRC_DIR}")
    add_subdirectory("${UENG_LLAMA_SRC_DIR}" "${CMAKE_BINARY_DIR}/llama.cpp" EXCLUDE_FROM_ALL)
    # Modern llama.cpp exports 'llama' target; headers provide <llama.h>
    add_compile_definitions(HAVE_LLAMA_H)
  else()
    message(WARNING "[ueng] UENG_WITH_LLAMA_EMBED=ON but third_party/llama.cpp not found; building with stubs.")
  endif()
endif()

# The wrapper 'src/llm_llama.c' is compiled into our executable unconditionally.
# If llama is present, link it.
if(TARGET llama)
  target_compile_definitions(uaengine PRIVATE UENG_WITH_LLAMA_EMBED=1)
  target_link_libraries(uaengine PRIVATE llama)
endif()
