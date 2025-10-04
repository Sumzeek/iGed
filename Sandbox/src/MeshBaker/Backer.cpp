module;
#include "meshoptimizer.h"
#include "tinyexr/tinyexr.h"
#include "xatlas.h"
#include <cstdlib>
#include <cstring>

module MeshBaker;
import :Baker;
import :CPUBaker;
import :OptixBaker;
import std;

namespace MeshBaker
{
std::shared_ptr<Baker> Baker::Create() {
    switch (s_Type) {
        case Type::None:
            throw std::runtime_error("Baker type not set.");
        case Type::CPU:
            return std::make_shared<CPUBaker>();
        case Type::Optix:
            return std::make_shared<OptixBaker>();
    }

    return nullptr;
}

bool Baker::RayTriangleIntersect(const glm::vec3& origin, const glm::vec3& dir, const glm::vec3& v0,
                                 const glm::vec3& v1, const glm::vec3& v2, float& t) {
    glm::vec3 o = origin - dir * 1e-6f; // Offset origin slightly along -normal to trigger hit when t=0

    //const float EPSILON = 1e-6f;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(dir, edge2);
    float a = glm::dot(edge1, h);
    if (std::abs(a) < 0.0f) { return false; }

    float f = 1.0f / a;
    glm::vec3 s = o - v0;
    float u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f) { return false; }

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(dir, q);
    if (v < 0.0f || u + v > 1.0f) { return false; }

    t = f * glm::dot(edge2, q);
    return t > 0.0f;
}

glm::vec3 Baker::ComputeBarycentric(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c, const glm::vec2& p) {
    glm::vec2 v0 = b - a;
    glm::vec2 v1 = c - a;
    glm::vec2 v2 = p - a;

    float d00 = glm::dot(v0, v0);
    float d01 = glm::dot(v0, v1);
    float d11 = glm::dot(v1, v1);
    float d20 = glm::dot(v2, v0);
    float d21 = glm::dot(v2, v1);

    float denom = d00 * d11 - d01 * d01;
    if (denom == 0.0f) { return glm::vec3{-1.0f, -1.0f, -1.0f}; }

    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;

    return glm::vec3(u, v, w);
}

void ApplySimplePadding(const BakeData& bakeData, std::vector<float>& displace, int n) {
    uint32_t width_u = bakeData.Width;
    uint32_t height_u = bakeData.Height;
    int width = static_cast<int>(width_u);
    int height = static_cast<int>(height_u);

    std::vector<uint8_t> mask(width * height, 0);
    for (int i = 0; i < width * height; ++i) {
        if (bakeData.BakedIndices[i] != BakedInvalidData) mask[i] = 1;
    }

    const int dirs[4][2] = {{0, 1}, {1, 0}, {0, -1}, {-1, 0}};
    for (int iter = 0; iter < n; ++iter) {
        std::vector<float> newDisplace = displace;
        std::vector<uint8_t> newMask = mask;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = y * width + x;
                if (mask[idx] == 1) continue;

                float sum = 0.0f;
                int count = 0;

                for (int d = 0; d < 4; ++d) {
                    int nx = x + dirs[d][0];
                    int ny = y + dirs[d][1];

                    if (nx < 0 || ny < 0 || nx >= width || ny >= height) continue;

                    int nidx = ny * width + nx;
                    if (mask[nidx] == 1) {
                        sum += displace[nidx];
                        count++;
                    }
                }

                if (count > 0) {
                    newDisplace[idx] = sum / count;
                    newMask[idx] = 1;
                }
            }
        }

        displace.swap(newDisplace);
        mask.swap(newMask);
    }
}

void ApplyTrianglePadding(const BakeData& bakeData, std::vector<float>& displace, int n) {
    uint32_t width = bakeData.Width;
    uint32_t height = bakeData.Height;

    std::vector<uint8_t> mask(width * height, 0);
    for (int i = 0; i < width * height; i++) {
        if (bakeData.BakedIndices[i] != BakedInvalidData) mask[i] = 1;
    }

    for (int iter = 0; iter < n; iter++) {
        std::vector<uint8_t> newMask = mask;
        std::vector<float> newDisplace = displace;

        for (int y = 0; y < (int) height; y++) {
            for (int x = 0; x < (int) width; x++) {
                int idx = y * width + x;
                if (mask[idx] == 1) continue;

                float value = 0.0f;
                int count = 0;

                //const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
                const int dirs[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
                for (auto& d: dirs) {
                    int nx = x + d[0];
                    int ny = y + d[1];
                    if (nx < 0 || ny < 0 || nx >= (int) width || ny >= (int) height) continue;

                    int nidx = ny * width + nx;
                    if (mask[nidx] == 1) {
                        value += displace[nidx];
                        count++;
                    }
                }

                if (count > 0) {
                    newDisplace[idx] = value / count;
                    newMask[idx] = 1;
                }
            }
        }

        displace.swap(newDisplace);
        mask.swap(newMask);
    }
}

glm::vec2 ClosestPointOnSegment(const glm::vec2& p, const glm::vec2& a, const glm::vec2& b) {
    glm::vec2 ab = b - a;
    float t = glm::dot(p - a, ab) / glm::dot(ab, ab);
    t = glm::clamp(t, 0.0f, 1.0f);
    return a + t * ab;
}
void ApplyTrianglePaddingStrict(const BakeData& bakeData, std::vector<float>& displace, int n) {
    uint32_t width = bakeData.Width;
    uint32_t height = bakeData.Height;

    std::vector<uint8_t> mask(width * height, 0);
    for (int i = 0; i < width * height; i++) {
        if (bakeData.BakedIndices[i] != BakedInvalidData) mask[i] = 1;
    }

    struct BoundaryPixel {
        int x, y;
        uint32_t triId;
    };
    std::vector<BoundaryPixel> boundaries;

    for (int y = 0; y < (int) height; y++) {
        for (int x = 0; x < (int) width; x++) {
            int idx = y * width + x;
            if (mask[idx] == 0) continue;

            bool isBoundary = false;
            const int dirs[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
            for (auto& d: dirs) {
                int nx = x + d[0], ny = y + d[1];
                if (nx < 0 || ny < 0 || nx >= (int) width || ny >= (int) height) continue;
                int nidx = ny * width + nx;
                if (mask[nidx] == 0) {
                    isBoundary = true;
                    break;
                }
            }
            if (isBoundary) { boundaries.push_back({x, y, bakeData.BakedIndices[idx]}); }
        }
    }

    for (int iter = 0; iter < n; iter++) {
        std::vector<uint8_t> newMask = mask;
        std::vector<float> newDisplace = displace;

        for (int y = 0; y < (int) height; y++) {
            for (int x = 0; x < (int) width; x++) {
                int idx = y * width + x;
                if (mask[idx] == 1) continue;

                glm::vec2 p(x + 0.5f, y + 0.5f);

                float bestDist2 = 1e9f;
                float bestValue = 0.0f;

                for (auto& bp: boundaries) {
                    int bidx = bp.y * width + bp.x;

                    uint32_t i0 = bakeData.Mesh2->Indices[bp.triId * 3 + 0];
                    uint32_t i1 = bakeData.Mesh2->Indices[bp.triId * 3 + 1];
                    uint32_t i2 = bakeData.Mesh2->Indices[bp.triId * 3 + 2];

                    glm::vec2 uv0 = bakeData.Mesh2->Vertices[i0].TexCoord * glm::vec2(width, height);
                    glm::vec2 uv1 = bakeData.Mesh2->Vertices[i1].TexCoord * glm::vec2(width, height);
                    glm::vec2 uv2 = bakeData.Mesh2->Vertices[i2].TexCoord * glm::vec2(width, height);

                    glm::vec2 c0 = ClosestPointOnSegment(p, uv0, uv1);
                    glm::vec2 c1 = ClosestPointOnSegment(p, uv1, uv2);
                    glm::vec2 c2 = ClosestPointOnSegment(p, uv2, uv0);

                    float d0 = glm::length(p - c0);
                    float d1 = glm::length(p - c1);
                    float d2 = glm::length(p - c2);

                    float d = std::min(d0, std::min(d1, d2));

                    if (d < bestDist2) {
                        bestDist2 = d;
                        bestValue = displace[bidx];
                    }
                }

                if (bestDist2 < 1e8f) {
                    newDisplace[idx] = bestValue;
                    newMask[idx] = 1;
                }
            }
        }

        displace.swap(newDisplace);
        mask.swap(newMask);
    }
}

void SaveExrFile(const BakeData& bakeData, const std::vector<float>& pixels) {
    const float* data = pixels.data();

    EXRHeader header;
    InitEXRHeader(&header);

    EXRImage image;
    InitEXRImage(&image);

    image.num_channels = 1;

    std::vector<float> channel(data, data + bakeData.Width * bakeData.Height);
    float* image_ptr[1];
    image_ptr[0] = channel.data();
    image.images = (unsigned char**) image_ptr;

    image.width = bakeData.Width;
    image.height = bakeData.Height;

    header.num_channels = 1;
    header.channels = (EXRChannelInfo*) malloc(sizeof(EXRChannelInfo) * header.num_channels);
    strncpy(header.channels[0].name, "Displacement", 255);
    header.channels[0].name[strlen("Displacement")] = '\0';

    header.pixel_types = (int*) malloc(sizeof(int) * header.num_channels);
    header.requested_pixel_types = (int*) malloc(sizeof(int) * header.num_channels);
    header.pixel_types[0] = TINYEXR_PIXELTYPE_FLOAT;
    header.requested_pixel_types[0] = TINYEXR_PIXELTYPE_FLOAT;

    const char* err = nullptr;
    std::string filename = "assets/textures/" + bakeData.Mesh2->Name + "_displacement.exr";
    int ret = SaveEXRImageToFile(&image, &header, filename.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        if (err) {
            std::cerr << "Save EXR error: " << err << "\n";
            FreeEXRErrorMessage(err);
        }
        free(header.channels);
        free(header.pixel_types);
        free(header.requested_pixel_types);
        return;
    }

    free(header.channels);
    free(header.pixel_types);
    free(header.requested_pixel_types);
}

std::vector<float> ReadExrFile(const std::string& filename, int& width, int& height) {
    EXRImage image;
    InitEXRImage(&image);

    EXRHeader header;
    InitEXRHeader(&header);

    const char* err = nullptr;

    EXRVersion version;
    ParseEXRVersionFromFile(&version, filename.c_str());

    int ret = ParseEXRHeaderFromFile(&header, &version, filename.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        std::cerr << "ParseEXRHeaderFromFile error: " << (err ? err : "unknown") << std::endl;
        if (err) FreeEXRErrorMessage(err);
        throw std::runtime_error("Failed to parse EXR header");
    }

    ret = LoadEXRImageFromFile(&image, &header, filename.c_str(), &err);
    if (ret != TINYEXR_SUCCESS) {
        std::cerr << "LoadEXRImageFromFile error: " << (err ? err : "unknown") << std::endl;
        if (err) FreeEXRErrorMessage(err);
        FreeEXRHeader(&header);
        throw std::runtime_error("Failed to load EXR image");
    }

    width = image.width;
    height = image.height;

    float* channel_ptr = reinterpret_cast<float*>(image.images[0]);

    std::vector<float> pixels(channel_ptr, channel_ptr + width * height);

    FreeEXRImage(&image);
    FreeEXRHeader(&header);

    return pixels;
}

void Bake(const Mesh& mesh1, const Mesh& mesh2, int resolution) {
    auto baker = Baker::Create();
    BakeData bakeData = baker->Bake(mesh1, mesh2, resolution);

    uint32_t width = bakeData.Width;
    uint32_t height = bakeData.Height;

    // Generate the data from BakeData
    std::vector<float> displace(width * height, 0.0f);
    {
        for (int i = 0; i < width * height; ++i) {
            uint32_t triId = bakeData.BakedIndices[i];
            if (triId == BakedInvalidData) {
                displace[i] = 0.0f;
                continue;
            }

            uint32_t i0 = bakeData.Mesh2->Indices[triId * 3 + 0];
            uint32_t i1 = bakeData.Mesh2->Indices[triId * 3 + 1];
            uint32_t i2 = bakeData.Mesh2->Indices[triId * 3 + 2];

            const glm::vec3& v0 = bakeData.Mesh2->Vertices[i0].Position;
            const glm::vec3& v1 = bakeData.Mesh2->Vertices[i1].Position;
            const glm::vec3& v2 = bakeData.Mesh2->Vertices[i2].Position;

            Baker::RayTriangleIntersect(bakeData.Originals[i], bakeData.Directions[i], v0, v1, v2, displace[i]);
        }
    }

    // Save exr file for displacement map
    SaveExrFile(bakeData, displace);
}

using Edge = std::pair<uint32_t, uint32_t>;
struct edge_hash {
    std::size_t operator()(const Edge& e) const noexcept {
        std::size_t h1 = std::hash<uint32_t>{}(e.first);
        std::size_t h2 = std::hash<uint32_t>{}(e.second);
        return h1 ^ (h2 << 1);
    }
};
void Bake(const Mesh& mesh, int resolution) {
    // Simplification mesh
    Mesh simMesh = SimplifyMesh(mesh);

    // Split to meshlet
    Mesh uvMesh;
    {
        uvMesh.Name = simMesh.Name + "_UV";
        uvMesh.Center = simMesh.Center;
        uvMesh.Radius = simMesh.Radius;

        // Build clusters using meshlets
        const size_t maxVerts = 64;
        const size_t maxTris = 124;
        const float coneWeight = 0.0f;

        std::vector<glm::vec3> positions = simMesh.GetPositionArray();
        const float* vertex_positions = reinterpret_cast<const float*>(positions.data());
        size_t vertex_count = positions.size();

        std::vector<uint32_t> indices = simMesh.GetIndexArray();
        size_t index_count = indices.size();

        std::vector<uint32_t> optimized_indices(index_count);
        meshopt_optimizeVertexCache(optimized_indices.data(), indices.data(), index_count, vertex_count);

        size_t max_meshlets = meshopt_buildMeshletsBound(index_count, maxVerts, maxTris);

        std::vector<meshopt_Meshlet> meshlets(max_meshlets);
        std::vector<uint32_t> meshletVertices(max_meshlets * maxVerts);
        std::vector<unsigned char> meshletTriangles(max_meshlets * maxTris * 3);

        size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshletVertices.data(), meshletTriangles.data(),
                                                     optimized_indices.data(), index_count, vertex_positions,
                                                     vertex_count, sizeof(float) * 3, maxVerts, maxTris, coneWeight);

        // Create new mesh from meshlets
        for (size_t i = 0; i < meshlet_count; ++i) {
            const meshopt_Meshlet& meshlet = meshlets[i];

            uint32_t offset = static_cast<uint32_t>(uvMesh.Vertices.size());
            for (uint32_t j = 0; j < meshlet.vertex_count; ++j) {
                uint32_t originalIndex = meshletVertices[meshlet.vertex_offset + j];
                uvMesh.Vertices.push_back(simMesh.Vertices[originalIndex]);
            }

            for (uint32_t t = 0; t < meshlet.triangle_count; ++t) {
                unsigned char* tri = &meshletTriangles[meshlet.triangle_offset + t * 3];
                for (int k = 0; k < 3; ++k) {
                    uint32_t originalIndex = offset + tri[k];
                    uvMesh.Indices.push_back(originalIndex);
                }
            }
        }
    }

    // Use xatlas to generate UV
    xatlas::Atlas* atlas = xatlas::Create();
    {
        xatlas::MeshDecl meshDecl{};
        meshDecl.vertexCount = static_cast<uint32_t>(uvMesh.Vertices.size());
        meshDecl.vertexPositionData = uvMesh.Vertices.data();
        meshDecl.vertexPositionStride = sizeof(Vertex);
        meshDecl.vertexUvData = nullptr;
        meshDecl.vertexUvStride = 0;

        meshDecl.indexCount = static_cast<uint32_t>(uvMesh.Indices.size());
        meshDecl.indexData = uvMesh.Indices.data();
        meshDecl.indexFormat = xatlas::IndexFormat::UInt32;

        meshDecl.faceCount = static_cast<uint32_t>(uvMesh.Indices.size() / 3);
        meshDecl.epsilon = 1e-6f;

        // Add mesh to xatlas
        xatlas::AddMesh(atlas, meshDecl);

        // Compute charts
        xatlas::ChartOptions chartOptions{};
        chartOptions.maxIterations = 8;
        chartOptions.useInputMeshUvs = false;
        xatlas::ComputeCharts(atlas, chartOptions);

        // Pack charts
        xatlas::PackOptions packOptions{};
        packOptions.resolution = resolution;
        packOptions.padding = 2;
        packOptions.bilinear = true;
        xatlas::PackCharts(atlas, packOptions);

        // Draw uv to uvMesh
        if (atlas->meshCount > 0) {
            const xatlas::Mesh& outMesh = atlas->meshes[0];
            for (uint32_t i = 0; i < outMesh.vertexCount; ++i) {
                const xatlas::Vertex& v = outMesh.vertexArray[i];
                if (v.atlasIndex >= 0) {
                    uvMesh.Vertices[v.xref].TexCoord[0] = v.uv[0] / static_cast<float>(atlas->width);
                    uvMesh.Vertices[v.xref].TexCoord[1] = v.uv[1] / static_cast<float>(atlas->height);
                }
            }
        }
    }
    xatlas::Destroy(atlas);

    //Mesh testMesh;
    //testMesh.Name = "sss";
    //
    //xatlas::Atlas* atlas = xatlas::Create();
    //{
    //    testMesh.Vertices.push_back(Vertex{.Position = {-1.0f, -1.0f, 0.0f}});
    //    testMesh.Vertices.push_back(Vertex{.Position = {-1.0f, 1.0f, 0.0f}});
    //    testMesh.Vertices.push_back(Vertex{.Position = {1.0f, -1.0f, 0.0f}});
    //    testMesh.Vertices.push_back(Vertex{.Position = {-1.0f, 1.0f, 0.0f}});
    //    testMesh.Vertices.push_back(Vertex{.Position = {1.0f, -1.0f, 0.0f}});
    //    testMesh.Vertices.push_back(Vertex{.Position = {1.0f, 1.0f, 0.0f}});
    //
    //    testMesh.Indices.push_back(0);
    //    testMesh.Indices.push_back(1);
    //    testMesh.Indices.push_back(2);
    //    testMesh.Indices.push_back(3);
    //    testMesh.Indices.push_back(4);
    //    testMesh.Indices.push_back(5);
    //
    //    xatlas::MeshDecl meshDecl{};
    //    //meshDecl.vertexCount = static_cast<uint32_t>(uvMesh.Vertices.size());
    //    meshDecl.vertexCount = static_cast<uint32_t>(testMesh.Vertices.size());
    //    //meshDecl.vertexPositionData = uvMesh.Vertices.data();
    //    meshDecl.vertexPositionData = testMesh.Vertices.data();
    //    meshDecl.vertexPositionStride = sizeof(Vertex);
    //    meshDecl.vertexUvData = nullptr;
    //    meshDecl.vertexUvStride = 0;
    //
    //    //meshDecl.indexCount = static_cast<uint32_t>(uvMesh.Indices.size());
    //    meshDecl.indexCount = static_cast<uint32_t>(testMesh.Indices.size());
    //    //meshDecl.indexData = uvMesh.Indices.data();
    //    meshDecl.indexData = testMesh.Indices.data();
    //    meshDecl.indexFormat = xatlas::IndexFormat::UInt32;
    //
    //    //meshDecl.faceCount = static_cast<uint32_t>(uvMesh.Indices.size() / 3);
    //    meshDecl.faceCount = static_cast<uint32_t>(2);
    //    meshDecl.epsilon = 1e-6f;
    //
    //    // Add mesh to xatlas
    //    xatlas::AddMesh(atlas, meshDecl);
    //
    //    // Compute charts
    //    xatlas::ChartOptions chartOptions{};
    //    chartOptions.maxIterations = 8;
    //    chartOptions.useInputMeshUvs = false;
    //    xatlas::ComputeCharts(atlas, chartOptions);
    //
    //    // Pack charts
    //    xatlas::PackOptions packOptions{};
    //    packOptions.resolution = resolution;
    //    packOptions.padding = 2;
    //    packOptions.bilinear = true;
    //    xatlas::PackCharts(atlas, packOptions);
    //
    //    // Draw uv to uvMesh
    //    if (atlas->meshCount > 0) {
    //        const xatlas::Mesh& outMesh = atlas->meshes[0];
    //        for (uint32_t i = 0; i < outMesh.vertexCount; ++i) {
    //            const xatlas::Vertex& v = outMesh.vertexArray[i];
    //            if (v.atlasIndex >= 0) {
    //                //uvMesh.Vertices[v.xref].TexCoord[0] = v.uv[0] / static_cast<float>(atlas->width);
    //                //uvMesh.Vertices[v.xref].TexCoord[1] = v.uv[1] / static_cast<float>(atlas->height);
    //                testMesh.Vertices[v.xref].TexCoord[0] = v.uv[0] / static_cast<float>(atlas->width);
    //                testMesh.Vertices[v.xref].TexCoord[1] = v.uv[1] / static_cast<float>(atlas->height);
    //            }
    //        }
    //    }
    //}
    //xatlas::Destroy(atlas);
    //ExportMeshAsOBJ(testMesh);

    //// Mark seams
    //std::vector<Edge> seams;
    //{
    //    // Build clusters using meshlets
    //    const size_t maxVerts = 64;
    //    const size_t maxTris = 124;
    //    const float coneWeight = 0.0f;
    //
    //    std::vector<glm::vec3> positions = simMesh.GetPositionArray();
    //    const float* vertex_positions = reinterpret_cast<const float*>(positions.data());
    //    size_t vertex_count = positions.size();
    //
    //    std::vector<uint32_t> indices = simMesh.GetIndexArray();
    //    size_t index_count = indices.size();
    //
    //    std::vector<uint32_t> optimized_indices(index_count);
    //    meshopt_optimizeVertexCache(optimized_indices.data(), indices.data(), index_count, vertex_count);
    //
    //    size_t max_meshlets = meshopt_buildMeshletsBound(index_count, maxVerts, maxTris);
    //
    //    std::vector<meshopt_Meshlet> meshlets(max_meshlets);
    //    std::vector<uint32_t> meshletVertices(max_meshlets * maxVerts);
    //    std::vector<unsigned char> meshletTriangles(max_meshlets * maxTris * 3);
    //
    //    size_t meshlet_count = meshopt_buildMeshlets(meshlets.data(), meshletVertices.data(), meshletTriangles.data(),
    //                                                 optimized_indices.data(), index_count, vertex_positions,
    //                                                 vertex_count, sizeof(float) * 3, maxVerts, maxTris, coneWeight);
    //
    //    // Use edge map to find seams
    //    auto make_edge = [](uint32_t a, uint32_t b) { return std::make_pair(std::min(a, b), std::max(a, b)); };
    //
    //    for (uint32_t m = 0; m < meshlet_count; ++m) {
    //        const meshopt_Meshlet& meshlet = meshlets[m];
    //
    //        size_t tri_start = meshlet.triangle_offset;
    //        size_t tri_end = tri_start + meshlet.triangle_count;
    //
    //        std::unordered_map<Edge, std::vector<uint32_t>, edge_hash> edgeToMeshlets;
    //        for (size_t t = tri_start; t < tri_end; ++t) {
    //            uint32_t i0 = meshletTriangles[t * 3 + 0];
    //            uint32_t i1 = meshletTriangles[t * 3 + 1];
    //            uint32_t i2 = meshletTriangles[t * 3 + 2];
    //
    //            edgeToMeshlets[make_edge(i0, i1)].push_back(m);
    //            edgeToMeshlets[make_edge(i1, i2)].push_back(m);
    //            edgeToMeshlets[make_edge(i2, i0)].push_back(m);
    //        }
    //
    //        for (auto& [edge, meshletList]: edgeToMeshlets) {
    //            std::sort(meshletList.begin(), meshletList.end());
    //            meshletList.erase(std::unique(meshletList.begin(), meshletList.end()), meshletList.end());
    //            if (meshletList.size() > 1) { seams.push_back(edge); }
    //        }
    //    }
    //}

    // Save the simplified mesh with UVs
    ExportMeshAsOBJ(uvMesh);
}

Mesh SimplifyMesh(const Mesh& src, float reductionRatio, float errorThreshold) {
    Mesh dst;
    dst.Name = src.Name + "_Sim";

    const auto& vertices = src.Vertices;
    const auto& indices = src.Indices;

    size_t indexCount = indices.size();
    size_t vertexCount = vertices.size();

    std::vector<uint32_t> newIndices(indexCount);

    size_t targetIndexCount = size_t(indexCount * reductionRatio);

    size_t resultCount =
            meshopt_simplify(newIndices.data(), indices.data(), indices.size(), (const float*) vertices.data(),
                             vertexCount, sizeof(Vertex), targetIndexCount, errorThreshold);

    newIndices.resize(resultCount);

    std::vector<uint32_t> remap(vertexCount);
    size_t newVertexCount = meshopt_generateVertexRemap(remap.data(), newIndices.data(), newIndices.size(),
                                                        vertices.data(), vertexCount, sizeof(Vertex));

    std::vector<Vertex> newVertices(newVertexCount);
    meshopt_remapVertexBuffer(newVertices.data(), vertices.data(), vertexCount, sizeof(Vertex), remap.data());
    meshopt_remapIndexBuffer(newIndices.data(), newIndices.data(), newIndices.size(), remap.data());

    dst.Vertices = std::move(newVertices);
    dst.Indices = std::move(newIndices);

    glm::vec3 center(0.0f);
    for (auto& v: dst.Vertices) center += v.Position;
    center /= (float) dst.Vertices.size();
    dst.Center = center;

    float radius = 0.0f;
    for (auto& v: dst.Vertices) radius = std::max(radius, glm::length(v.Position - center));
    dst.Radius = radius;

    return dst;
}
} // namespace MeshBaker
