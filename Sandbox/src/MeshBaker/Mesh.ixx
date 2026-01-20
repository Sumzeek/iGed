module;

export module iGed.MeshBaker:Mesh;
import std;
import glm;

namespace MeshBaker
{

export struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    // float Curvature;
    // glm::vec3 Tangent;
    // glm::vec3 BiTangent;
};

// Edge structure for quad mesh edge mapping (position-based)
export struct Edge {
    glm::vec3 p0; // First vertex position (smaller in lexicographic order)
    glm::vec3 p1; // Second vertex position (larger in lexicographic order)

    // Compare two vec3 lexicographically
    static bool LessThan(const glm::vec3& a, const glm::vec3& b) {
        constexpr float eps = 1e-6f;
        if (std::abs(a.x - b.x) > eps) return a.x < b.x;
        if (std::abs(a.y - b.y) > eps) return a.y < b.y;
        if (std::abs(a.z - b.z) > eps) return a.z < b.z;
        return false; // Equal
    }

    static bool Equal(const glm::vec3& a, const glm::vec3& b) {
        constexpr float eps = 1e-6f;
        return std::abs(a.x - b.x) <= eps &&
               std::abs(a.y - b.y) <= eps &&
               std::abs(a.z - b.z) <= eps;
    }

    Edge(const glm::vec3& a, const glm::vec3& b) {
        // Ensure p0 < p1 in lexicographic order for consistent hashing
        if (LessThan(a, b)) {
            p0 = a;
            p1 = b;
        } else {
            p0 = b;
            p1 = a;
        }
    }

    bool operator==(const Edge& other) const {
        return Equal(p0, other.p0) && Equal(p1, other.p1);
    }
};

// Hash function for Edge (position-based)
export struct EdgeHash {
    std::size_t operator()(const Edge& e) const {
        // Quantize positions to avoid floating point hash issues
        auto quantize = [](float v) -> std::int64_t {
            return static_cast<std::int64_t>(std::round(v * 1e5));
        };

        std::size_t h = 0;
        // Hash p0
        h ^= std::hash<std::int64_t>{}(quantize(e.p0.x)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<std::int64_t>{}(quantize(e.p0.y)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<std::int64_t>{}(quantize(e.p0.z)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        // Hash p1
        h ^= std::hash<std::int64_t>{}(quantize(e.p1.x)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<std::int64_t>{}(quantize(e.p1.y)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<std::int64_t>{}(quantize(e.p1.z)) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

// Quad edge mapping data
export struct QuadEdgeMapping {
    std::vector<glm::uvec4> QuadEdgeIds; // Each quad's 4 edge IDs (bottom, right, top, left)
    std::uint32_t EdgeCount;             // Total number of unique edges

    // Build edge mapping from quad mesh indices using vertex positions
    // Assumes indices are organized as quads: [v0, v1, v2, v3] per quad
    // Quad vertex order: v0(bottom-left), v1(bottom-right), v2(top-right), v3(top-left)
    // Edge order: bottom(v0-v1), right(v1-v2), top(v2-v3), left(v3-v0)
    static QuadEdgeMapping Build(const std::vector<std::uint32_t>& indices,
                                  const std::vector<Vertex>& vertices) {
        QuadEdgeMapping mapping;
        std::uint32_t quadCount = static_cast<std::uint32_t>(indices.size() / 4);
        mapping.QuadEdgeIds.resize(quadCount);

        std::unordered_map<Edge, std::uint32_t, EdgeHash> edgeToId;
        std::uint32_t nextEdgeId = 0;

        auto getOrCreateEdgeId = [&](const glm::vec3& posA, const glm::vec3& posB) -> std::uint32_t {
            Edge edge(posA, posB);
            auto it = edgeToId.find(edge);
            if (it != edgeToId.end()) { return it->second; }
            std::uint32_t id = nextEdgeId++;
            edgeToId[edge] = id;
            return id;
        };

        for (std::uint32_t q = 0; q < quadCount; ++q) {
            std::uint32_t i0 = indices[q * 4 + 0]; // bottom-left
            std::uint32_t i1 = indices[q * 4 + 1]; // bottom-right
            std::uint32_t i2 = indices[q * 4 + 2]; // top-right
            std::uint32_t i3 = indices[q * 4 + 3]; // top-left

            const glm::vec3& p0 = vertices[i0].Position;
            const glm::vec3& p1 = vertices[i1].Position;
            const glm::vec3& p2 = vertices[i2].Position;
            const glm::vec3& p3 = vertices[i3].Position;

            // Get edge IDs for each edge of the quad using positions
            std::uint32_t bottomEdgeId = getOrCreateEdgeId(p0, p1);
            std::uint32_t rightEdgeId = getOrCreateEdgeId(p1, p2);
            std::uint32_t topEdgeId = getOrCreateEdgeId(p2, p3);
            std::uint32_t leftEdgeId = getOrCreateEdgeId(p3, p0);

            mapping.QuadEdgeIds[q] = glm::uvec4(bottomEdgeId, rightEdgeId, topEdgeId, leftEdgeId);
        }

        mapping.EdgeCount = nextEdgeId;
        return mapping;
    }
};

export struct Mesh {
    std::string Name;
    std::vector<Vertex> Vertices;
    std::vector<std::uint32_t> Indices;

    glm::vec3 Center;
    float Radius;

    std::vector<glm::vec3> GetPositionArray() const {
        int size = Vertices.size();

        std::vector<glm::vec3> positions(size);
        for (int i = 0; i < size; ++i) { positions[i] = Vertices[i].Position; }

        return positions;
    }

    std::vector<std::uint32_t> GetIndexArray() const { return Indices; }

    // Build quad edge mapping for this mesh
    QuadEdgeMapping BuildQuadEdgeMapping() const { return QuadEdgeMapping::Build(Indices, Vertices); }
};

export Mesh LoadObjFile(const std::filesystem::path& filepath);

export void FillCurvature(Mesh& mesh, int w, int h, const std::vector<float>& displacementValues);

export void ExportMeshAsOBJ(const Mesh& mesh);
} // namespace MeshBaker
