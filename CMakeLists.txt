cmake_minimum_required(VERSION 3.16)
project(uaengine LANGUAGES C)

# ---- C standard ----
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

# ---- Sources ----
add_executable(uaengine
  src/main.c
  src/common.c
  src/fs.c
  src/serve.c 
)

# ---- Includes ----
# Public: headers under include/ueng so consumers can do #include "ueng/…"
# Private: src for any internal includes
target_include_directories(uaengine
  PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# ---- Libraries ----
if(WIN32)
  target_link_libraries(uaengine PRIVATE ws2_32)
endif()

# ---- Warnings / defines ----
if(MSVC)
  target_compile_definitions(uaengine PRIVATE _CRT_SECURE_NO_WARNINGS)
  target_compile_options(uaengine PRIVATE /W3)
else()
  target_compile_options(uaengine PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# ---- Install (binary + public headers + optional theme) ----
install(TARGETS uaengine RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install public headers under include/ueng
install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ueng/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ueng
  FILES_MATCHING PATTERN "*.h"
)

# Optional starter theme
install(FILES themes/uae.css DESTINATION ${CMAKE_INSTALL_DATADIR}/uaengine/themes OPTIONAL)
# Optional example script