module;
#define NOMINMAX
#include "optix9.h"

#include "myStruct.h"

module MeshBaker;
import :OptixBaker;
import std;

namespace MeshBaker
{
template<typename T>
struct __align__(OPTIX_SBT_RECORD_ALIGNMENT) SbtRecord {
    __align__(OPTIX_SBT_RECORD_ALIGNMENT) char header[OPTIX_SBT_RECORD_HEADER_SIZE];
    T data;
};

using RayGenSbtRecord = SbtRecord<RayGenData>;
using HitSbtRecord = SbtRecord<ClosestHitData>;
using MissSbtRecord = SbtRecord<MissData>;

static std::string LoadPTX(const char* filename) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Failed to open PTX file: " << filename << std::endl;
        exit(-1);
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

static void ContextLogCB(unsigned int level, const char* tag, const char* message, void* /*cbdata */) {
    std::cerr << "[" << std::setw(2) << level << "][" << std::setw(12) << tag << "]: " << message << "\n";
}

BakeData OptixBaker::Bake(const Mesh& mesh1, const Mesh& mesh2, int resolution) {
    BakeData bakeData{};
    bakeData.Mesh1 = &mesh1;
    bakeData.Mesh2 = &mesh2;
    bakeData.Width = resolution;
    bakeData.Height = resolution;
    bakeData.Originals.resize(resolution * resolution);
    bakeData.Directions.resize(resolution * resolution);
    bakeData.BakedIndices.resize(resolution * resolution, BakedInvalidData);

    // Prepare launch data
    for (size_t i = 0; i < mesh1.Indices.size(); i += 4) {
        uint32_t i0 = mesh1.Indices[i];
        uint32_t i1 = mesh1.Indices[i + 1];
        uint32_t i2 = mesh1.Indices[i + 2];
        uint32_t i3 = mesh1.Indices[i + 3];

        glm::vec3 p0 = mesh1.Vertices[i0].Position;
        glm::vec3 p1 = mesh1.Vertices[i1].Position;
        glm::vec3 p2 = mesh1.Vertices[i2].Position;
        glm::vec3 p3 = mesh1.Vertices[i3].Position;

        glm::vec3 n0 = mesh1.Vertices[i0].Normal;
        glm::vec3 n1 = mesh1.Vertices[i1].Normal;
        glm::vec3 n2 = mesh1.Vertices[i2].Normal;
        glm::vec3 n3 = mesh1.Vertices[i3].Normal;

        glm::vec2 uv0 = mesh1.Vertices[i0].TexCoord;
        glm::vec2 uv1 = mesh1.Vertices[i1].TexCoord;
        glm::vec2 uv2 = mesh1.Vertices[i2].TexCoord;
        glm::vec2 uv3 = mesh1.Vertices[i3].TexCoord;

        glm::ivec2 pixel0 = uv0;
        glm::ivec2 pixel1 = uv1;
        glm::ivec2 pixel2 = uv2;
        glm::ivec2 pixel3 = uv3;

        int minX = std::max(0, std::min({pixel0.x, pixel1.x, pixel2.x, pixel3.x}));
        int maxX = std::min(resolution - 1, std::max({pixel0.x, pixel1.x, pixel2.x, pixel3.x}));
        int minY = std::max(0, std::min({pixel0.y, pixel1.y, pixel2.y, pixel3.y}));
        int maxY = std::min(resolution - 1, std::max({pixel0.y, pixel1.y, pixel2.y, pixel3.y}));

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                float u = float(x - minX) / float(maxX - minX);
                float v = float(y - minY) / float(maxY - minY);

                glm::vec3 posOnMesh1 = (1 - u) * (1 - v) * p0 + u * (1 - v) * p1 + u * v * p2 + (1 - u) * v * p3;
                glm::vec3 norOnMesh1 =
                        glm::normalize((1 - u) * (1 - v) * n0 + u * (1 - v) * n1 + u * v * n2 + (1 - u) * v * n3);

                int idx = y * resolution + x;
                bakeData.Originals[idx] = posOnMesh1 - 1e-6f * norOnMesh1; // Add a small epsilon to origin
                bakeData.Directions[idx] = norOnMesh1;
            }
        }
    }

    char log[2048];
    size_t logSize = sizeof(log);

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
    OptixTraversableHandle gasHandle;
    CUdeviceptr d_vertices;
    CUdeviceptr d_indices;
    CUdeviceptr d_tempBufferGas;
    CUdeviceptr d_gasOutputBuffer;
    {
        OptixAccelBuildOptions accelOptions = {};
        accelOptions.buildFlags = OPTIX_BUILD_FLAG_ALLOW_COMPACTION;
        accelOptions.operation = OPTIX_BUILD_OPERATION_BUILD;
        accelOptions.motionOptions.numKeys = 1;
        accelOptions.motionOptions.timeBegin = 0.0f;
        accelOptions.motionOptions.timeEnd = 1.0f;
        accelOptions.motionOptions.flags = OPTIX_MOTION_FLAG_NONE;

        std::vector<glm::vec3> vertices = mesh2.GetPositionArray();
        const size_t verticesSize = vertices.size() * sizeof(glm::vec3);
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_vertices), verticesSize));
        CUDA_CHECK(
                cudaMemcpy(reinterpret_cast<void*>(d_vertices), vertices.data(), verticesSize, cudaMemcpyHostToDevice));

        std::vector<uint32_t> indices = mesh2.GetIndexArray();
        const size_t indicesSize = indices.size() * sizeof(uint32_t);
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_indices), indicesSize));
        CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_indices), indices.data(), indicesSize, cudaMemcpyHostToDevice));

        OptixBuildInput buildInput = {};
        buildInput.type = OPTIX_BUILD_INPUT_TYPE_TRIANGLES;
        buildInput.triangleArray.vertexFormat = OPTIX_VERTEX_FORMAT_FLOAT3;
        buildInput.triangleArray.vertexStrideInBytes = sizeof(float3);
        buildInput.triangleArray.numVertices = vertices.size();
        buildInput.triangleArray.vertexBuffers = &d_vertices;

        buildInput.triangleArray.indexFormat = OPTIX_INDICES_FORMAT_UNSIGNED_INT3;
        buildInput.triangleArray.indexStrideInBytes = sizeof(uint3);
        buildInput.triangleArray.numIndexTriplets = indices.size() / 3;
        buildInput.triangleArray.indexBuffer = d_indices;

        uint32_t triangleInputFlags = OPTIX_GEOMETRY_FLAG_DISABLE_ANYHIT;
        buildInput.triangleArray.flags = &triangleInputFlags;
        buildInput.triangleArray.numSbtRecords = 1;

        OptixAccelBufferSizes gasBufferSizes;
        OPTIX_CHECK(optixAccelComputeMemoryUsage(context, &accelOptions, &buildInput,
                                                 1, // num_build_inputs
                                                 &gasBufferSizes));

        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_tempBufferGas), gasBufferSizes.tempSizeInBytes));
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_gasOutputBuffer), gasBufferSizes.outputSizeInBytes));

        OPTIX_CHECK(optixAccelBuild(context,
                                    0, // CUDA stream
                                    &accelOptions, &buildInput,
                                    1, // num build inputs
                                    d_tempBufferGas, gasBufferSizes.tempSizeInBytes, d_gasOutputBuffer,
                                    gasBufferSizes.outputSizeInBytes, &gasHandle, nullptr, 0));
    }

    // Create module
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

        // load ptx file
        std::string ptx = LoadPTX("assets/ptxs/optixKernel_75.ptx");

        OPTIX_CHECK(optixModuleCreate(context, &moduleCompileOptions, &pipelineCompileOptions, ptx.c_str(), ptx.size(),
                                      log, &logSize, &module));
    }

    // Create program groups, including NULL miss and hitgroups
    OptixProgramGroup raygenPG = nullptr;
    OptixProgramGroup hitPG = nullptr;
    OptixProgramGroup missPG = nullptr;
    {
        OptixProgramGroupOptions programGroupOptions = {};

        OptixProgramGroupDesc raygenProgramGroupDesc = {};
        raygenProgramGroupDesc.kind = OPTIX_PROGRAM_GROUP_KIND_RAYGEN;
        raygenProgramGroupDesc.raygen.module = module;
        raygenProgramGroupDesc.raygen.entryFunctionName = "__raygen__rg";
        OPTIX_CHECK(optixProgramGroupCreate(context, &raygenProgramGroupDesc, 1, &programGroupOptions, log, &logSize,
                                            &raygenPG));

        OptixProgramGroupDesc hitProgramGroupDesc = {};
        hitProgramGroupDesc.kind = OPTIX_PROGRAM_GROUP_KIND_HITGROUP;
        hitProgramGroupDesc.hitgroup.moduleCH = module;
        hitProgramGroupDesc.hitgroup.entryFunctionNameCH = "__closesthit__ch";
        OPTIX_CHECK(
                optixProgramGroupCreate(context, &hitProgramGroupDesc, 1, &programGroupOptions, log, &logSize, &hitPG));

        OptixProgramGroupDesc missProgramGroupDesc = {};
        missProgramGroupDesc.kind = OPTIX_PROGRAM_GROUP_KIND_MISS;
        missProgramGroupDesc.miss.module = module;
        missProgramGroupDesc.miss.entryFunctionName = "__miss__ms";
        OPTIX_CHECK(optixProgramGroupCreate(context, &missProgramGroupDesc, 1, &programGroupOptions, log, &logSize,
                                            &missPG));
    }

    // Link pipeline
    OptixPipeline pipeline = nullptr;
    {
        OptixPipelineLinkOptions pipelineLinkOptions = {};
        pipelineLinkOptions.maxTraceDepth = 1;
        OptixProgramGroup programGroups[] = {raygenPG, missPG, hitPG};
        OPTIX_CHECK(optixPipelineCreate(context, &pipelineCompileOptions, &pipelineLinkOptions, programGroups, 3, log,
                                        &logSize, &pipeline));
    }

    // Set up shader binding table
    OptixShaderBindingTable sbt = {};
    {
        CUdeviceptr raygenRecord;
        const size_t raygenRecordSize = sizeof(RayGenSbtRecord);
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&raygenRecord), raygenRecordSize));
        RayGenSbtRecord rg_sbt{};
        OPTIX_CHECK(optixSbtRecordPackHeader(raygenPG, &rg_sbt));
        CUDA_CHECK(
                cudaMemcpy(reinterpret_cast<void*>(raygenRecord), &rg_sbt, raygenRecordSize, cudaMemcpyHostToDevice));

        CUdeviceptr hitRecord;
        const size_t hitRecordSize = sizeof(HitSbtRecord);
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&hitRecord), hitRecordSize));
        HitSbtRecord hit_sbt;
        OPTIX_CHECK(optixSbtRecordPackHeader(hitPG, &hit_sbt));
        CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(hitRecord), &hit_sbt, hitRecordSize, cudaMemcpyHostToDevice));

        CUdeviceptr missRecord;
        size_t missRecordSize = sizeof(MissSbtRecord);
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&missRecord), missRecordSize));
        MissSbtRecord ms_sbt;
        OPTIX_CHECK(optixSbtRecordPackHeader(missPG, &ms_sbt));
        CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(missRecord), &ms_sbt, missRecordSize, cudaMemcpyHostToDevice));

        sbt.raygenRecord = raygenRecord;
        sbt.hitgroupRecordBase = hitRecord;
        sbt.hitgroupRecordStrideInBytes = sizeof(HitSbtRecord);
        sbt.hitgroupRecordCount = 1;
        sbt.missRecordBase = missRecord;
        sbt.missRecordStrideInBytes = sizeof(MissSbtRecord);
        sbt.missRecordCount = 1;
    }

    // launch
    {
        CUstream stream;
        CUDA_CHECK(cudaStreamCreate(&stream));

        uint32_t count = bakeData.Width * bakeData.Height;

        CUdeviceptr d_rays;
        std::vector<Ray> rays(count);
        for (uint32_t i = 0; i < count; ++i) {
            glm::vec3 origin = bakeData.Originals[i];
            glm::vec3 direction = bakeData.Directions[i];
            rays[i] =
                    Ray{make_float3(origin.x, origin.y, origin.z), make_float3(direction.x, direction.y, direction.z)};
        }
        const size_t raysSize = rays.size() * sizeof(Ray);
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_rays), raysSize));
        CUDA_CHECK(cudaMemcpy(reinterpret_cast<void*>(d_rays), rays.data(), raysSize, cudaMemcpyHostToDevice));

        CUdeviceptr d_visBuffer;
        const size_t visBufferSize = count * sizeof(uint32_t);
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_visBuffer), visBufferSize));

        LaunchParams params{};
        params.Handle = gasHandle;
        params.Rays = reinterpret_cast<Ray*>(d_rays);
        params.VisBuffer = reinterpret_cast<uint32_t*>(d_visBuffer);
        params.Width = bakeData.Width;
        params.Height = bakeData.Height;

        CUdeviceptr d_params;
        CUDA_CHECK(cudaMalloc(reinterpret_cast<void**>(&d_params), sizeof(LaunchParams)));
        CUDA_CHECK(
                cudaMemcpy(reinterpret_cast<void*>(d_params), &params, sizeof(LaunchParams), cudaMemcpyHostToDevice));

        OPTIX_CHECK(
                optixLaunch(pipeline, stream, d_params, sizeof(LaunchParams), &sbt, params.Width, params.Height, 1));

        CUDA_CHECK(cudaStreamSynchronize(stream));
        std::println("OptiX 9 bake finished.");

        cudaMemcpy(bakeData.BakedIndices.data(), (void*) d_visBuffer, visBufferSize, cudaMemcpyDeviceToHost);

        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_rays)));
        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_visBuffer)));
        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_params)));
    }

    // Cleanup
    {
        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_vertices)));
        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_indices)));
        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_tempBufferGas)));
        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(d_gasOutputBuffer)));
        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(sbt.raygenRecord)));
        CUDA_CHECK(cudaFree(reinterpret_cast<void*>(sbt.missRecordBase)));
        optixPipelineDestroy(pipeline);
        optixProgramGroupDestroy(raygenPG);
        optixModuleDestroy(module);
        optixDeviceContextDestroy(context);
    }

    return bakeData;
}
} // namespace MeshBaker
