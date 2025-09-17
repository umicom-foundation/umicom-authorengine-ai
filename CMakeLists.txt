cmake_minimum_required(VERSION 3.20)
project(uaengine C)

set(CMAKE_C_STANDARD 17)

add_executable(uaengine
  src/main.c
  src/common.c
  src/fs.c
)

target_include_directories(uaengine PRIVATE ${CMAKE_SOURCE_DIR}/include)

if (WIN32)
  target_link_libraries(uaengine ws2_32)
endif()


target_include_directories(uaengine PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# near the bottom of CMakeLists.txt
install(TARGETS uaengine RUNTIME DESTINATION bin)

# optional: install starter theme
install(FILES themes/uae.css DESTINATION share/uaengine/themes OPTIONAL)
