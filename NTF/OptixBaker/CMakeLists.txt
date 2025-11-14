cmake_minimum_required(VERSION 3.30 FATAL_ERROR)
project(OptixBaker LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(3rdparty)

find_package(CUDAToolkit REQUIRED)
set(OPTIX_DIR "C:/ProgramData/NVIDIA Corporation/OptiX SDK 9.0.0")
set(OPTIX_INCLUDE_DIR "${OPTIX_DIR}/include")

pybind11_add_module(optixbaker src/OptixBaker.cpp)

target_include_directories(optixbaker PRIVATE
        ${CUDAToolkit_INCLUDE_DIRS}
        ${OPTIX_INCLUDE_DIR}
)
target_link_libraries(optixbaker PRIVATE
        CUDA::cudart
        CUDA::cuda_driver
)

target_compile_definitions(optixbaker PRIVATE NOMINMAX)

if (WIN32)
    set_target_properties(optixbaker PROPERTIES
            PREFIX ""
            SUFFIX ".pyd"
            OUTPUT_NAME "optixbaker"
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/
    )
endif ()
