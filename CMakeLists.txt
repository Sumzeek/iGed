cmake_minimum_required(VERSION 3.29 FATAL_ERROR)

project(iGed VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

add_compile_definitions(_UTF8)

# enable modules support
#if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
#    add_compile_options(/experimental:module /std:c++latest)
#elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
#    add_compile_options(-fmodules-ts -std=c++23)
#endif ()

set(IGED_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(iGe)
add_subdirectory(SandBox)
