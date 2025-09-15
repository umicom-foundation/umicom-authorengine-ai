cmake_minimum_required(VERSION 3.20)

# Project: uaengine (Umicom AuthorEngine) — C-first, portable, fast
project(uaengine VERSION 0.1.0 LANGUAGES C)

# Use modern, portable C
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(uaengine
    src/main.c
)

target_include_directories(uaengine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# near the bottom of CMakeLists.txt
install(TARGETS uaengine RUNTIME DESTINATION bin)

# optional: install starter theme
install(FILES themes/uae.css DESTINATION share/uaengine/themes OPTIONAL)
