module;

export module MeshBaker:Mesh;
import std;
import glm;

namespace MeshBaker
{

export struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    // glm::vec3 Tangent;
    // glm::vec3 BiTangent;
};

export struct Mesh {
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

    std::vector<uint32_t> GetIndexArray() const { return Indices; }
};

export Mesh LoadObjFile(std::string const& path);

} // namespace MeshBaker
