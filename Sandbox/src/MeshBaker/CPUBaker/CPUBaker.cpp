module;

module MeshBaker;
import :CPUBaker;
import std;
import glm;

namespace MeshBaker
{
BakeData CPUBaker::Bake(const Mesh& mesh1, const Mesh& mesh2, int resolution) {
    auto HasTexCoords = [&](const Mesh& mesh) -> bool {
        for (const auto& vertex: mesh.Vertices) {
            if (vertex.TexCoord != glm::vec2(0.0f, 0.0f)) { return true; }
        }
        return false;
    };
    if (!HasTexCoords(mesh1)) {
        throw std::runtime_error("Bake failed: origin mesh is missing texture coordinates (UVs).");
    }

    BakeData bakeData{};
    bakeData.Mesh1 = &mesh1;
    bakeData.Mesh2 = &mesh2;
    bakeData.Width = resolution;
    bakeData.Height = resolution;
    bakeData.Originals.resize(resolution * resolution);
    bakeData.Directions.resize(resolution * resolution);
    bakeData.BakedIndices.resize(resolution * resolution, BakedInvalidData);

    for (size_t i = 0; i < mesh1.Indices.size(); i += 4) {
        std::uint32_t i0 = mesh1.Indices[i];
        std::uint32_t i1 = mesh1.Indices[i + 1];
        std::uint32_t i2 = mesh1.Indices[i + 2];
        std::uint32_t i3 = mesh1.Indices[i + 3];

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
                bakeData.Originals[idx] = posOnMesh1;
                bakeData.Directions[idx] = norOnMesh1;
            }
        }
    }

    return bakeData;
}

std::uint32_t CPUBaker::IntersectAlongNormal(const Mesh& mesh, const glm::vec3& origin, const glm::vec3& normal) {
    float minT = std::numeric_limits<float>::max();
    std::uint32_t triID = BakedInvalidData;

    for (size_t i = 0; i < mesh.Indices.size(); i += 3) {
        glm::vec3 v0 = mesh.Vertices[mesh.Indices[i]].Position;
        glm::vec3 v1 = mesh.Vertices[mesh.Indices[i + 1]].Position;
        glm::vec3 v2 = mesh.Vertices[mesh.Indices[i + 2]].Position;

        float t;
        glm::vec3 p;
        if (RayTriangleIntersect(origin, normal, v0, v1, v2, t, p)) {
            if (std::abs(t) < std::abs(minT)) {
                minT = std::abs(t);
                triID = static_cast<std::uint32_t>(i / 3);
            }
        }

        if (RayTriangleIntersect(origin, -normal, v0, v1, v2, t, p)) {
            if (std::abs(t) < std::abs(minT)) {
                minT = -std::abs(t);
                triID = static_cast<std::uint32_t>(i / 3);
            }
        }
    }

    return triID;
}

} // namespace MeshBaker
