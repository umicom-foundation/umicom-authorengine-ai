cmake_minimum_required(VERSION 3.20)
project(ueng VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ueng src/main.cpp)
target_include_directories(ueng PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
