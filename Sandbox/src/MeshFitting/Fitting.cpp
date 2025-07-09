module;
#include "iGeMacro.h"
#include <stb_image_write.h>

module MeshFitting;
import :Fitting;
import std;

namespace MeshFitting
{

static float SaveDisplacementMapAsPNG(const DisplacementMap& displacement, const std::string& filename) {
    const int pixelCount = displacement.Width * displacement.Height;
    const float* src = displacement.Data.data();

    // Compute displacement map scale
    float maxAbs = 0.0f;
    for (int i = 0; i < pixelCount; ++i) { maxAbs = std::max(maxAbs, std::abs(src[i])); }
    float scale = (maxAbs < 1e-6f) ? 1.0f : maxAbs;

    // Map data to [0, 255] and fill in RGB channels
    std::vector<unsigned char> image(pixelCount * 4);
    for (int i = 0; i < pixelCount; ++i) {
        float v = src[i];
        float mapped = 0.5f + (v / (2.0f * scale));
        mapped = std::clamp(mapped, 0.0f, 1.0f);
        unsigned char value = static_cast<unsigned char>(mapped * 255.0f);

        image[i * 4 + 0] = value;
        image[i * 4 + 1] = value;
        image[i * 4 + 2] = value;
        image[i * 4 + 3] = 255;
    }

    // Write PNG file
    if (!stbi_write_png(filename.c_str(), displacement.Width, displacement.Height, 4, image.data(),
                        displacement.Width * 4)) {
        IGE_ERROR("Failed to write PNG file: {}", filename);
        return -1.0f;
    }

    return scale;
}

static glm::vec3 ComputeBarycentric(const glm::vec2& a, const glm::vec2& b, const glm::vec2& c, const glm::vec2& p) {
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

static bool RayTriangleIntersect(const glm::vec3& origin, const glm::vec3& dir, const glm::vec3& v0,
                                 const glm::vec3& v1, const glm::vec3& v2, float& tOut, glm::vec3& hitPosOut) {
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

    float t = f * glm::dot(edge2, q);
    //if (t > EPSILON) {
    if (t >= 0) {
        hitPosOut = origin + dir * t;
        tOut = t;
        return true;
    }

    return false;
}

float IntersectAlongNormal(const Mesh& mesh, const glm::vec3& origin, const glm::vec3& normal) {
    float minT = std::numeric_limits<float>::max();
    glm::vec3 bestHit = origin;

    for (size_t i = 0; i < mesh.Indices.size(); i += 3) {
        glm::vec3 v0 = mesh.Vertices[mesh.Indices[i]].Position;
        glm::vec3 v1 = mesh.Vertices[mesh.Indices[i + 1]].Position;
        glm::vec3 v2 = mesh.Vertices[mesh.Indices[i + 2]].Position;

        float t;
        glm::vec3 hit;
        if (RayTriangleIntersect(origin, normal, v0, v1, v2, t, hit)) {
            if (std::abs(t) < std::abs(minT)) { minT = t; }
        }

        if (RayTriangleIntersect(origin, -normal, v0, v1, v2, t, hit)) {
            if (std::abs(t) < std::abs(minT)) { minT = -t; }
        }
    }

    return minT;
}

iGe::Ref<iGe::Texture2D> GenerateDisplacementMap(const Mesh& mesh1, const Mesh& mesh2, int resolution) {
    auto HasTexCoords = [&](const Mesh& mesh) -> bool {
        for (const auto& vertex: mesh.Vertices) {
            if (vertex.TexCoord != glm::vec2(0.0f, 0.0f)) { return true; }
        }
        return false;
    };
    if (!HasTexCoords(mesh1)) {
        IGE_ERROR("Displacement map generation failed: origin mesh is missing texture coordinates (UVs).");
    }

    DisplacementMap map{};
    map.Width = resolution;
    map.Height = resolution;
    map.Data.resize(resolution * resolution, 0.0f);

    for (size_t i = 0; i < mesh1.Indices.size(); i += 3) {
        uint32_t i0 = mesh1.Indices[i];
        uint32_t i1 = mesh1.Indices[i + 1];
        uint32_t i2 = mesh1.Indices[i + 2];

        glm::vec3 p0 = mesh1.Vertices[i0].Position;
        glm::vec3 p1 = mesh1.Vertices[i1].Position;
        glm::vec3 p2 = mesh1.Vertices[i2].Position;

        glm::vec3 n0 = mesh1.Vertices[i0].Normal;
        glm::vec3 n1 = mesh1.Vertices[i1].Normal;
        glm::vec3 n2 = mesh1.Vertices[i2].Normal;

        glm::vec2 uv0 = mesh1.Vertices[i0].TexCoord;
        glm::vec2 uv1 = mesh1.Vertices[i1].TexCoord;
        glm::vec2 uv2 = mesh1.Vertices[i2].TexCoord;

        glm::vec2 pixel0 = uv0 * float(resolution - 1);
        glm::vec2 pixel1 = uv1 * float(resolution - 1);
        glm::vec2 pixel2 = uv2 * float(resolution - 1);

        int minX = std::max(0, int(std::floor(std::min({pixel0.x, pixel1.x, pixel2.x}))));
        int maxX = std::min(resolution - 1, int(std::ceil(std::max({pixel0.x, pixel1.x, pixel2.x}))));
        int minY = std::max(0, int(std::floor(std::min({pixel0.y, pixel1.y, pixel2.y}))));
        int maxY = std::min(resolution - 1, int(std::ceil(std::max({pixel0.y, pixel1.y, pixel2.y}))));

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                glm::vec2 p = glm::vec2(x, y);

                // Compute barycentric
                glm::vec3 bary = ComputeBarycentric(pixel0, pixel1, pixel2, p);
                if (bary.x < 0 || bary.y < 0 || bary.z < 0) continue;

                // Compute point and normal after interpolation
                glm::vec3 posOnMesh1 = bary.x * p0 + bary.y * p1 + bary.z * p2;
                glm::vec3 norOnMesh1 = glm::normalize(bary.x * n0 + bary.y * n1 + bary.z * n2);

                // Project to mesh2
                float offset = IntersectAlongNormal(mesh2, posOnMesh1, norOnMesh1);

                int idx = y * resolution + x;
                map.Data[idx] = offset;
            }
        }
    }

    float scale = SaveDisplacementMapAsPNG(map, "displacement.png");
    IGE_INFO("Generate Displacement map: {}, scale = {}.", "displacement.png", scale);

    iGe::TextureSpecification specification;
    specification.Width = resolution;
    specification.Height = resolution;
    specification.Format = iGe::ImageFormat::R32F;
    specification.GenerateMips = false;

    iGe::Ref<iGe::Texture2D> texture = iGe::Texture2D::Create(specification);
    texture->SetData(map.Data.data(), map.Data.size() * sizeof(float));
    return texture;
}

} // namespace MeshFitting