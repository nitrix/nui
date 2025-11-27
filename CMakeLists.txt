cmake_minimum_required(VERSION 3.16)
project(nui)

# =====================================
# CMake dependencies
# =====================================
add_subdirectory(vendor/glad)
add_subdirectory(vendor/glfw)

# =====================================
# NUI library
# =====================================
add_library(nui STATIC nui/nui.c)
target_include_directories(nui PUBLIC nui)

# =====================================
# NGL Backend
# =====================================
add_library(backend-ngl STATIC backends/ngl/ngl.c)
target_include_directories(backend-ngl PUBLIC backends/ngl)
target_include_directories(backend-ngl PRIVATE vendor/stb)
target_link_libraries(backend-ngl PRIVATE glad)
target_link_libraries(backend-ngl PUBLIC nui)

if(unix)
    target_link_libraries(backend-ngl PRIVATE m)
endif()

# =====================================
# SW Backend
# =====================================
add_library(backend-sw STATIC backends/sw/sw.c)
target_include_directories(backend-sw PUBLIC backends/sw)
target_include_directories(backend-sw PRIVATE vendor/stb)
target_link_libraries(backend-sw PUBLIC nui)

if(unix)
    target_link_libraries(backend-sw PRIVATE m)
endif()

# =====================================
# Example
# =====================================
add_executable(example main.c)
target_link_libraries(example nui backend-ngl glad glfw)
target_compile_options(example PRIVATE -std=c23 -Wall -Wextra -pedantic)

# target_compile_options(example PRIVATE -fsanitize=address,undefined)
# target_link_options(example PRIVATE -fsanitize=address,undefined)

# =====================================
# Testing
# =====================================
enable_testing()

file(GLOB testfiles_fullpath "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.c")
SET(testfiles_relative)

foreach(name ${testfiles_fullpath})
    string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/tests/" "" name ${name})
    SET(testfiles_relative ${testfiles_relative} ${name})
endforeach()

create_test_sourcelist(testlist test_runner.c ${testfiles_relative})
add_executable(test_runner test_runner.c ${testfiles_fullpath})
target_link_libraries(test_runner nui backend-sw)

foreach(name ${testfiles_relative})
    string(REPLACE ".c" "" name ${name})
    add_test(NAME ${name} COMMAND test_runner ${name} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/tests)
endforeach()