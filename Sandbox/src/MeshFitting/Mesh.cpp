module;
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

module MeshFitting;
import :Mesh;
import glm;
import std;

namespace MeshFitting
{

Mesh LoadObjFile(std::string const& path) {
    Mesh mesh;

    // read file via assimp
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                                                           aiProcess_GenBoundingBoxes | aiProcess_ForceGenNormals |
                                                           aiProcess_GenSmoothNormals);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        throw std::runtime_error(std::format("Assimp Error: {}", importer.GetErrorString()));
    }

    // assuming the file has only one grid
    aiMesh* aiMesh = scene->mMeshes[0];

    auto& vertices = mesh.Vertices;
    auto& normals = mesh.Normals;
    for (int i = 0; i < aiMesh->mNumVertices; ++i) {
        auto vertex = aiMesh->mVertices[i];
        vertices.push_back(glm::vec3{vertex.x, vertex.y, vertex.z});

        auto normal = aiMesh->mNormals[i];
        normals.push_back(glm::vec3{normal.x, normal.y, normal.z});
    }

    auto& indices = mesh.Indices;
    for (unsigned int i = 0; i < aiMesh->mNumFaces; i++) {
        aiFace face = aiMesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) { indices.push_back(face.mIndices[j]); }
    }

    // update bounding-box
    glm::vec3 minBounds = glm::vec3{aiMesh->mAABB.mMin.x, aiMesh->mAABB.mMin.y, aiMesh->mAABB.mMin.z};
    glm::vec3 maxBounds = glm::vec3{aiMesh->mAABB.mMax.x, aiMesh->mAABB.mMax.y, aiMesh->mAABB.mMax.z};
    mesh.Center = (minBounds + maxBounds) * 0.5f;
    mesh.Radius = glm::length(maxBounds - minBounds) * 0.5f;

    return mesh;
}

} // namespace MeshFitting
