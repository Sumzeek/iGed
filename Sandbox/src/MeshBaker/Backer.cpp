module;
#include "tinyexr/tinyexr.h"
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
    const float EPSILON = 1e-6f;
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(dir, edge2);
    float a = glm::dot(edge1, h);
    if (std::abs(a) < EPSILON) { return false; }

    float f = 1.0f / a;
    glm::vec3 s = origin - v0;
    float u = f * glm::dot(s, h);
    if (u < 0.0f || u > 1.0f) { return false; }

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(dir, q);
    if (v < 0.0f || u + v > 1.0f) { return false; }

    t = f * glm::dot(edge2, q);
    return t > EPSILON;
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
    int ret = SaveEXRImageToFile(&image, &header, "displacement.exr", &err);
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
//std::vector<float> ReadExrFile(const std::string& filePath, int& width, int& height) {
//    float* out;
//    const char* err = nullptr;
//
//    int ret = LoadEXR(&out, &width, &height, filePath.c_str(), &err);
//
//    if (ret != TINYEXR_SUCCESS) {
//        std::string errMsg = err ? err : "Unknown error";
//        if (err) { FreeEXRErrorMessage(err); }
//        std::cout << "Failed to load EXR file: " << errMsg << std::endl;
//        throw std::runtime_error("Failed to load EXR file: " + errMsg);
//    }
//
//    std::vector<float> pixels(out, out + (width * height));
//    return pixels;
//}

void Bake(const Mesh& mesh1, const Mesh& mesh2, int resolution) {
    auto baker = Baker::Create();
    BakeData bakeData = baker->Bake(mesh1, mesh2, resolution);

    // Generate the data from BakeData
    std::vector<float> displace(bakeData.Width * bakeData.Height, 0.0f);
    {
        for (int i = 0; i < bakeData.Width * bakeData.Height; ++i) {
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
} // namespace MeshBaker