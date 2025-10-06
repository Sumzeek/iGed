cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

# https://github.com/Kitware/CMake/blob/master/Help/dev/experimental.rst
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")
set(CMAKE_CXX_MODULE_STD ON)

project(iGed VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(_UTF8)

# enable modules support
#if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
#    add_compile_options(/experimental:module /std:c++latest)
#elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
#    add_compile_options(-fmodules-ts -std=c++23)
#endif ()

set(IGED_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(iGe)
add_subdirectory(Sandbox)
