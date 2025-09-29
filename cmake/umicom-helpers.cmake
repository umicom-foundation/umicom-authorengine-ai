# ======================================================================
# Umicom Foundation - Build helpers
# Author: Sammy Hegab
# Organization: Umicom Foundation (https://umicom.foundation/)
# License: MIT
# Date: 2025-09-29
#
# PURPOSE:
# - Wire vcpkg-provided libcurl to the 'uaengine' target.
# - Provide a 'uengine' meta-target that builds 'uaengine' so that legacy
#   commands like `cmake --build . --target uengine` continue to work.
#
# Notes:
# - Requires that vcpkg toolchain is used when configuring CMake.
#   e.g. -DCMAKE_TOOLCHAIN_FILE=C:/Dev/vcpkg/scripts/buildsystems/vcpkg.cmake
# - Requires that vcpkg triplet x64-windows was used to install curl:
#   vcpkg install curl[ssl]:x64-windows
# ======================================================================

# Be quiet if included multiple times
if(DEFINED _UMICOM_HELPERS_INCLUDED)
  return()
endif()
set(_UMICOM_HELPERS_INCLUDED TRUE)

# Only proceed if the expected target exists
if(NOT TARGET uaengine)
  message(STATUS "umicom-helpers.cmake: target 'uaengine' not found; skip CURL wiring and alias.")
  return()
endif()

# Find libcurl via vcpkg's CMake config package and link it
find_package(CURL REQUIRED)
target_link_libraries(uaengine PRIVATE CURL::libcurl)

# Provide a convenience phony target named 'uengine' that depends on 'uaengine'.
# This lets users build with: cmake --build . --target uengine
if(NOT TARGET uengine)
  add_custom_target(uengine DEPENDS uaengine)
endif()

message(STATUS "umicom-helpers.cmake: linked CURL::libcurl to 'uaengine' and created 'uengine' convenience target.")
