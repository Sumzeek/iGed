#include <cuda_runtime.h>
#include <optix.h>

#include "myStruct.h"

// Compile for use "nvcc -I"C:/ProgramData/NVIDIA Corporation/OptiX SDK 9.0.0/include" --ptx bakeKernel.cu -o bakeKernel.ptx -arch=compute_75"
// Compile for use "nvcc -I"C:/ProgramData/NVIDIA Corporation/OptiX SDK 9.0.0/include" --ptx bakeKernel.cu -o bakeKernel.ptx -arch=compute_86"

__device__ float3 operator-(const float3& a) { return make_float3(-a.x, -a.y, -a.z); }

__device__ float3 operator+(const float3& a, const float3& b) { return make_float3(a.x + b.x, a.y + b.y, a.z + b.z); }

__device__ float3 operator-(const float3& a, const float3& b) { return make_float3(a.x - b.x, a.y - b.y, a.z - b.z); }

__device__ float3 operator*(const float3& a, float b) { return make_float3(a.x * b, a.y * b, a.z * b); }

__device__ float3 cross(const float3& a, const float3& b) {
    return make_float3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x);
}

__device__ float dot(const float3& a, const float3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

extern "C" {
__constant__ LaunchParams params;

extern "C" __global__ void __raygen__rg() {
    uint3 launchIndex = optixGetLaunchIndex();
    unsigned int idx = launchIndex.y * params.Width + launchIndex.x;

    Ray ray = params.Rays[idx];

    unsigned int p0_a, p1_a;
    optixTrace(params.Handle, ray.Origin, ray.Direction, 0.0f, 1e16f, 0.0f, OptixVisibilityMask(1), OPTIX_RAY_FLAG_NONE,
               0, 1, 0, p0_a, p1_a);
    float t_a = __uint_as_float(p0_a);

    unsigned int p0_b, p1_b;
    optixTrace(params.Handle, ray.Origin, -ray.Direction, 0.0f, 1e16f, 0.0f, OptixVisibilityMask(1),
               OPTIX_RAY_FLAG_NONE, 0, 1, 0, p0_b, p1_b);
    float t_b = __uint_as_float(p0_b);

    unsigned int result_id;
    if (t_a < 0.0f) {
        result_id = p1_b;
    } else if (t_b < 0.0f) {
        result_id = p1_a;
    } else {
        if (t_a < t_b) {
            result_id = p1_a;
        } else {
            result_id = p1_b;
        }
    }

    params.VisBuffer[idx] = result_id;
}

extern "C" __global__ void __closesthit__ch() {
    float t = optixGetRayTmax();

    optixSetPayload_0(__float_as_uint(t));
    optixSetPayload_1(optixGetPrimitiveIndex());
}

extern "C" __global__ void __miss__ms() {
    float t = -1.0f; // Miss ray, set t to -1

    optixSetPayload_0(__float_as_uint(t));
    optixSetPayload_1(0xFFFFFFFF); // Invalid primitive index
}

} // extern "C"
