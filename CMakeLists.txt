cmake_minimum_required(VERSION 3.20)

# Project: ueng (Umicom AuthorEngine) — C-first to keep it low-level and fast.
project(ueng VERSION 0.1.0 LANGUAGES C)

# We aim for portable, modern C. C17 is a good baseline (wide compiler support).
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)   # Prefer -std=c17 (no gnu extensions) for portability

# (Later) You can add platform-specific flags here if you need extra performance.

add_executable(ueng
    src/main.c
)

target_include_directories(ueng PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
