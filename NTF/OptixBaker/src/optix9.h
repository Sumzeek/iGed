#pragma once

#include <cuda_runtime.h>
#include <iostream>
#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stack_size.h>
#include <optix_stubs.h>

#define CUDA_CHECK(call)                                                                                               \
    do {                                                                                                               \
        cudaError_t err = call;                                                                                        \
        if (err != cudaSuccess) {                                                                                      \
            std::cerr << "CUDA error at " << __FILE__ << ":" << __LINE__ << ": " << cudaGetErrorString(err)            \
                      << std::endl;                                                                                    \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)

#define OPTIX_CHECK(call)                                                                                              \
    do {                                                                                                               \
        OptixResult res = call;                                                                                        \
        if (res != OPTIX_SUCCESS) {                                                                                    \
            std::cerr << "Optix error at " << __FILE__ << ":" << __LINE__ << ": " << optixGetErrorString(res)          \
                      << std::endl;                                                                                    \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)
