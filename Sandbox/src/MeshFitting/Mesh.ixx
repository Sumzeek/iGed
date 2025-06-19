//
// Created by Sumzeek on 6/9/2025.
//

export module MeshFitting:Mesh;
import glm;
import std;

namespace MeshFitting
{

export struct Mesh {
    std::vector<glm::vec3> Vertices;
    std::vector<glm::vec3> Normals;
    std::vector<std::uint32_t> Indices;

    glm::vec3 Center;
    float Radius;
};

export Mesh LoadObjFile(std::string const& path);

} // namespace MeshFitting