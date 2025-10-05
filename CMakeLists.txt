cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

# https://github.com/Kitware/CMake/blob/master/Help/dev/experimental.rst
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "a9e1cf81-9932-4810-974b-6eccaf14e457")
set(CMAKE_CXX_MODULE_STD ON)

if (CMAKE_CXX_COMPILER MATCHES "clang\\+\\+")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set(CMAKE_CXX_COMPILER_CLANG_SCAN_DEPS "/usr/lib/llvm-18/bin/clang-scan-deps")
endif ()

project(iGed VERSION 0.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_definitions(_UTF8)

set(IGED_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(iGe)
add_subdirectory(Sandbox)
