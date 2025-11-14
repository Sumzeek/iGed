#include <algorithm>
#include <cstdlib>
#include <cuda_runtime.h>
#include <fstream>
#include <iostream>
#include <optix.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <stdexcept>
#include <string>
#include <vector>

#include "myStruct.h"
#include "optix9.h"

namespace py = pybind11;

template<typename T>
struct __align__(OPTIX_SBT_RECORD_ALIGNMENT) SbtRecord {
    __align__(OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    T data;
};

using RayGenSbtRecord = SbtRecord<RayGenData>;
using HitSbtRecord = SbtRecord<ClosestHitData>;
using MissSbtRecord = SbtRecord<MissData>;

namespace
{
thread_local const char* g_print = "start";
inline void Print(const char* s) {
    g_print = s;
    std::cerr << "[optixbaker] print: " << s << std::endl;
}
} // namespace

static void ContextLogCB(unsigned int level, const char* tag, const char* message, void* /*cbdata*/) {
    std::cerr << "[" << level << "]" << tag << ": " << message << "\n";
}

static std::string LoadPTX(const std::string& path) {
    std::ifstream f(path, std::ios::in | std::ios::binary);
    if (!f) { throw std::runtime_error("Failed to open PTX file: " + path); }
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

py::array_t<int> intersect(py::array_t<float, py::array::c_style | py::array::forcecast> origins,
                           py::array_t<float, py::array::c_style | py::array::forcecast> directions,
                           py::array_t<float, py::array::c_style | py::array::forcecast> vertices,
                           py::array_t<unsigned int, py::array::c_style | py::array::forcecast> indices) {
    // Validate inputs
    auto o = origins.request();
    auto d = directions.request();
    if (o.ndim != 2 || d.ndim != 2 || o.shape[1] != 3 || d.shape[1] != 3 || o.shape[0] != d.shape[0]) {
        throw std::invalid_argument("origins and directions must be (N,3) arrays with the same N");
    }

    // Prepare vertex/index arrays, support vertices as (V,3) or (M,3,3)
    auto vinfo = vertices.request();
    auto iinfo = indices.request();

    std::vector<float3> h_vertices;
    std::vector<uint3> h_indices;

    if (vinfo.ndim == 2) {
        if (vinfo.shape[1] != 3) throw std::invalid_argument("vertices must have shape (V,3) or (M,3,3)");
        const size_t V = static_cast<size_t>(vinfo.shape[0]);
        const float* vptr = static_cast<const float*>(vinfo.ptr);
        h_vertices.resize(V);
        for (size_t i = 0; i < V; ++i) {
            h_vertices[i] = make_float3(vptr[3 * i + 0], vptr[3 * i + 1], vptr[3 * i + 2]);
        }
        if (iinfo.ndim != 2 || iinfo.shape[1] != 3) {
            throw std::invalid_argument("indices must have shape (T,3) when vertices is (V,3)");
        }
        const size_t T = static_cast<size_t>(iinfo.shape[0]);
        const unsigned int* iptr = static_cast<const unsigned int*>(iinfo.ptr);
        h_indices.resize(T);
        for (size_t t = 0; t < T; ++t) { h_indices[t] = make_uint3(iptr[3 * t + 0], iptr[3 * t + 1], iptr[3 * t + 2]); }
    } else if (vinfo.ndim == 3) {
        if (vinfo.shape[1] != 3 || vinfo.shape[2] != 3) {
            throw std::invalid_argument("vertices must have shape (V,3) or (M,3,3)");
        }
        const size_t M = static_cast<size_t>(vinfo.shape[0]);
        const float* vptr = static_cast<const float*>(vinfo.ptr);
        h_vertices.resize(M * 3);
        h_indices.resize(M);
        for (size_t t = 0; t < M; ++t) {
            // three vertices per triangle
            const size_t baseV = t * 9; // 3 * 3 floats
            const size_t baseIdx = t * 3;
            h_vertices[baseIdx + 0] = make_float3(vptr[baseV + 0], vptr[baseV + 1], vptr[baseV + 2]);
            h_vertices[baseIdx + 1] = make_float3(vptr[baseV + 3], vptr[baseV + 4], vptr[baseV + 5]);
            h_vertices[baseIdx + 2] = make_float3(vptr[baseV + 6], vptr[baseV + 7], vptr[baseV + 8]);
            h_indices[t] = make_uint3(static_cast<unsigned int>(baseIdx + 0), static_cast<unsigned int>(baseIdx + 1),
                                      static_cast<unsigned int>(baseIdx + 2));
        }
    } else {
        throw std::invalid_argument("vertices must have shape (V,3) or (M,3,3)");
    }

    const uint32_t N = static_cast<uint32_t>(o.shape[0]);

    // Initialize CUDA and create OptiX context
    OptixDeviceContext context = nullptr;
    {
        CUDA_CHECK(cudaFree(0));
        CUcontext cuCtx = 0;
        OPTIX_CHECK(optixInit());
        OptixDeviceContextOptions options = {};
        options.logCallbackFunction = &ContextLogCB;
        options.logCallbackLevel = 4;
        OPTIX_CHECK(optixDeviceContextCreate(cuCtx, &options, &context));
    }

    // Build triangle GAS
    OptixTraversableHandle gasHandle = {};
    CUdeviceptr d_vertices = 0;
    CUdeviceptr d_indices = 0;
    CUdeviceptr d_tempBufferGas = 0;
    CUdeviceptr d_gasOutputBuffer = 0;
    {
        OptixAccelBuildOptions accelOptions = {};
        accelOptions.buildFlags = OPTIX_BUILD_FLAG_ALLOW_COMPACTION;
        accelOptions.operation = OPTIX_BUILD_OPERATION_BUILD;

        // Upload geometry
        const size_t verticesSize = h_vertices.size() * sizeof(float3);
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_vertices), verticesSize));
        CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_vertices), h_vertices.data(), verticesSize,
                              cudaMemcpyHostToDevice));

        const size_t indicesSize = h_indices.size() * sizeof(uint3);
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_indices), indicesSize));
        CUDA_CHECK(
                cudaMemcpy(reinterpret_cast<void*>(d_indices), h_indices.data(), indicesSize, cudaMemcpyHostToDevice));

        OptixBuildInput buildInput = {};
        buildInput.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
        buildInput.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
        buildInput.triangleArray.vertexStrideInBytes = sizeof(float3);
        buildInput.triangleArray.numVertices = static_cast<unsigned int>(h_vertices.size());
        buildInput.triangleArray.vertexBuffers = &d_vertices;

        buildInput.triangleArray.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
        buildInput.triangleArray.indexStrideInBytes = sizeof(uint3);
        buildInput.triangleArray.numIndexTriplets = static_cast<unsigned int>(h_indices.size());
        buildInput.triangleArray.indexBuffer = d_indices;

        uint32_t triangleInputFlags = OPTIX_GEOMETRY_FLAG_DISABLE_ANYHIT;
        buildInput.triangleArray.flags = &triangleInputFlags;
        buildInput.triangleArray.numSbtRecords = 1;

        OptixAccelBufferSizes gasBufferSizes;
        OPTIX_CHECK(optixAccelComputeMemoryUsage(context, &accelOptions, &buildInput, 1, &gasBufferSizes));

        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_tempBufferGas), gasBufferSizes.tempSizeInBytes));
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_gasOutputBuffer), gasBufferSizes.outputSizeInBytes));

        OPTIX_CHECK(optixAccelBuild(context,
                                    0, // stream
                                    &accelOptions, &buildInput, 1, d_tempBufferGas, gasBufferSizes.tempSizeInBytes,
                                    d_gasOutputBuffer, gasBufferSizes.outputSizeInBytes, &gasHandle, nullptr, 0));
    }

    // Create module from PTX
    OptixModule module = nullptr;
    OptixPipelineCompileOptions pipelineCompileOptions = {};
    {
        OptixModuleCompileOptions moduleCompileOptions = {};
        pipelineCompileOptions.usesMotionBlur = false;
        pipelineCompileOptions.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_SINGLE_GAS;
        pipelineCompileOptions.numPayloadValues = 2;
        pipelineCompileOptions.numAttributeValues = 2;
        pipelineCompileOptions.exceptionFlags = OPTIX_EXCEPTION_FLAG_NONE;
        pipelineCompileOptions.pipelineLaunchParamsVariableName = "params";

        std::string ptx;
        const char* ptxEnv = std::getenv("OPTIX_INTERSECT_PTX");
        if (!ptxEnv) {
            std::cerr << "OPTIX_INTERSECT_PTX not set!\n";
        } else {
            ptx = LoadPTX(ptxEnv);
        }

        char log[2048];
        size_t logSize = sizeof(log);
        OPTIX_CHECK(optixModuleCreate(context, &moduleCompileOptions, &pipelineCompileOptions, ptx.c_str(), ptx.size(),
                                      log, &logSize, &module));
    }

    // Program groups
    OptixProgramGroup raygenPG = nullptr;
    OptixProgramGroup hitPG = nullptr;
    OptixProgramGroup missPG = nullptr;
    {
        OptixProgramGroupOptions programGroupOptions = {};
        char log[2048];
        size_t logSize = sizeof(log);

        OptixProgramGroupDesc raygenDesc = {};
        raygenDesc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
        raygenDesc.raygen.module = module;
        raygenDesc.raygen.entryFunctionName = "__raygen__rg";
        OPTIX_CHECK(optixProgramGroupCreate(context, &raygenDesc, 1, &programGroupOptions, log, &logSize, &raygenPG));

        OptixProgramGroupDesc hitDesc = {};
        hitDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
        hitDesc.hitgroup.moduleCH = module;
        hitDesc.hitgroup.entryFunctionNameCH = "__closesthit__ch";
        OPTIX_CHECK(optixProgramGroupCreate(context, &hitDesc, 1, &programGroupOptions, log, &logSize, &hitPG));

        OptixProgramGroupDesc missDesc = {};
        missDesc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
        missDesc.miss.module = module;
        missDesc.miss.entryFunctionName = "__miss__ms";
        OPTIX_CHECK(optixProgramGroupCreate(context, &missDesc, 1, &programGroupOptions, log, &logSize, &missPG));
    }

    // Pipeline
    OptixPipeline pipeline = nullptr;
    {
        OptixPipelineLinkOptions linkOptions = {};
        linkOptions.maxTraceDepth = 1;
        char log[2048];
        size_t logSize = sizeof(log);
        OptixProgramGroup groups[] = {raygenPG, missPG, hitPG};
        OPTIX_CHECK(optixPipelineCreate(context, &pipelineCompileOptions, &linkOptions, groups, 3, log, &logSize,
                                        &pipeline));
        // Conservative stack sizes for simple trace
        OPTIX_CHECK(optixPipelineSetStackSize(pipeline,
                                              /*directCallableStackSizeFromTraversal*/ 2 * 1024,
                                              /*directCallableStackSizeFromState*/ 2 * 1024,
                                              /*continuationStackSize*/ 2 * 1024,
                                              /*maxTraversableGraphDepth*/ 1));
    }

    // SBT
    OptixShaderBindingTable sbt = {};
    {
        CUdeviceptr d_raygenRecord = 0;
        RayGenSbtRecord rg = {};
        OPTIX_CHECK(optixSbtRecordPackHeader(raygenPG, &rg));
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_raygenRecord), sizeof(RayGenSbtRecord)));
        CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_raygenRecord), &rg, sizeof(RayGenSbtRecord),
                              cudaMemcpyHostToDevice));

        CUdeviceptr d_hitRecord = 0;
        HitSbtRecord hg = {};
        OPTIX_CHECK(optixSbtRecordPackHeader(hitPG, &hg));
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_hitRecord), sizeof(HitSbtRecord)));
        CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_hitRecord), &hg, sizeof(HitSbtRecord), cudaMemcpyHostToDevice));

        CUdeviceptr d_missRecord = 0;
        MissSbtRecord ms = {};
        OPTIX_CHECK(optixSbtRecordPackHeader(missPG, &ms));
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_missRecord), sizeof(MissSbtRecord)));
        CUDA_CHECK(
                cudaMemcpy(reinterpret_cast<void*>(d_missRecord), &ms, sizeof(MissSbtRecord), cudaMemcpyHostToDevice));

        sbt.raygenRecord = d_raygenRecord;
        sbt.hitgroupRecordBase = d_hitRecord;
        sbt.hitgroupRecordStrideInBytes = sizeof(HitSbtRecord);
        sbt.hitgroupRecordCount = 1;
        sbt.missRecordBase = d_missRecord;
        sbt.missRecordStrideInBytes = sizeof(MissSbtRecord);
        sbt.missRecordCount = 1;
    }

    // Launch
    py::array_t<int> result(N);
    {
        // Build rays buffer
        const float* optr = static_cast<const float*>(o.ptr);
        const float* dptr = static_cast<const float*>(d.ptr);
        std::vector<Ray> h_rays(N);
        for (uint32_t i = 0; i < N; ++i) {
            float3 ro = make_float3(optr[3 * i + 0], optr[3 * i + 1], optr[3 * i + 2]);
            float3 rd = make_float3(dptr[3 * i + 0], dptr[3 * i + 1], dptr[3 * i + 2]);
            h_rays[i] = {ro, rd};
        }

        CUdeviceptr d_rays = 0;
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_rays), sizeof(Ray) * N));
        CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_rays), h_rays.data(), sizeof(Ray) * N, cudaMemcpyHostToDevice));

        CUdeviceptr d_vis = 0;
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_vis), sizeof(uint32_t) * N));

        LaunchParams params = {};
        params.Handle = gasHandle;
        params.Rays = reinterpret_cast<Ray*>(d_rays);
        params.VisBuffer = reinterpret_cast<uint32_t*>(d_vis);
        params.Width = N;
        params.Height = 1;

        CUdeviceptr d_params = 0;
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_params), sizeof(LaunchParams)));
        CUDA_CHECK(
                cudaMemcpy(reinterpret_cast<void*>(d_params), &params, sizeof(LaunchParams), cudaMemcpyHostToDevice));

        CUstream stream;
        CUDA_CHECK(cudaStreamCreate(&stream));
        OPTIX_CHECK(
                optixLaunch(pipeline, stream, d_params, sizeof(LaunchParams), &sbt, params.Width, params.Height, 1));
        CUDA_CHECK(cudaStreamSynchronize(stream));

        // Copy back
        std::vector<uint32_t> h_vis(N);
        CUDA_CHECK(cudaMemcpy(h_vis.data(), reinterpret_cast<void const*>(d_vis), sizeof(uint32_t) * N,
                              cudaMemcpyDeviceToHost));
        auto r = result.mutable_unchecked<1>();
        for (uint32_t i = 0; i < N; ++i) { r(i) = (h_vis[i] == 0xFFFFFFFFu) ? -1 : static_cast<int>(h_vis[i]); }

        // Cleanup launch buffers
        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_rays)));
        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_vis)));
        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_params)));
        CUDA_CHECK(cudaStreamDestroy(stream));
    }

    // Cleanup static allocations & OptiX constructs
    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_vertices)));
    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_indices)));
    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_tempBufferGas)));
    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_gasOutputBuffer)));
    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(sbt.raygenRecord)));
    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(sbt.hitgroupRecordBase)));
    CUDA_CHECK(cudaFree(reinterpret_cast<void*>(sbt.missRecordBase)));
    optixPipelineDestroy(pipeline);
    optixProgramGroupDestroy(raygenPG);
    optixProgramGroupDestroy(hitPG);
    optixProgramGroupDestroy(missPG);
    optixModuleDestroy(module);
    optixDeviceContextDestroy(context);

    return result;
}

PYBIND11_MODULE(optixbaker, m) {
    m.doc() = "OptiX triangle intersection module";
    m.def("intersect", &intersect, py::arg("origins"), py::arg("directions"), py::arg("vertices"), py::arg("indices"),
          R"doc(Return primitive indices for ray-triangle intersections using NVIDIA OptiX.\n\nParameters:\n  origins: (N,3) float array of ray origins\n  directions: (N,3) float array of ray directions\n  triangles: (M,3,3) float array of triangle vertices\nReturns:\n  (N,) int array of primitive indices; -1 indicates a miss.\n\nEnvironment:\n  Set OPTIX_INTERSECT_PTX to override the path to optix_kernel.ptx if needed.)doc");
}
