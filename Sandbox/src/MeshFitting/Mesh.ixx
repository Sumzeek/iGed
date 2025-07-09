module;
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

export module MeshFitting:Mesh;
import iGe;

namespace MeshFitting
{

export struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    glm::vec3 Tangent;
    glm::vec3 BiTangent;
};

export struct Mesh {
    std::vector<Vertex> Vertices;
    std::vector<std::uint32_t> Indices;

    glm::vec3 Center;
    float Radius;
};

export Mesh LoadObjFile(std::string const& path);

} // namespace MeshFitting
