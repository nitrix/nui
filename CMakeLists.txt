cmake_minimum_required(VERSION 3.16)
project(nui)

add_subdirectory(vendor/glad)
add_subdirectory(vendor/glfw)

add_executable(example main.c)
target_link_libraries(example nui glad glfw)
target_compile_options(example PRIVATE -std=c23 -Wall -Wextra -pedantic)

add_library(nui STATIC nui.c nui.h ngl.c ngl.h)
target_link_libraries(nui PRIVATE glad)