cmake_minimum_required(VERSION 3.19 FATAL_ERROR)

project(iGed VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUILD_SHARED_LIBS OFF)

set(IGED_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

add_subdirectory(iGed)
add_subdirectory(app)
