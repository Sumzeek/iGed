module;
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

module MeshBaker;
import :Mesh;

namespace MeshBaker
{

Mesh LoadObjFile(std::string const& path) {
    Mesh mesh;

    // read file via assimp
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
                                                           aiProcess_GenBoundingBoxes | aiProcess_ForceGenNormals |
                                                           aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        throw std::runtime_error(std::format("Assimp Error: {}", importer.GetErrorString()));
    }

    // assuming the file has only one grid
    aiMesh* aiMesh = scene->mMeshes[0];

    auto& vertices = mesh.Vertices;
    for (uint32_t v = 0; v < aiMesh->mNumVertices; ++v) {
        Vertex vertex{};
        vertex.Position = {aiMesh->mVertices[v].x, aiMesh->mVertices[v].y, aiMesh->mVertices[v].z};
        if (aiMesh->HasNormals()) {
            vertex.Normal = {aiMesh->mNormals[v].x, aiMesh->mNormals[v].y, aiMesh->mNormals[v].z};
        } else {
            vertex.Normal = {0.0f, 0.0f, 0.0f};
        }
        if (aiMesh->mTextureCoords[0]) {
            vertex.TexCoord = {aiMesh->mTextureCoords[0][v].x, aiMesh->mTextureCoords[0][v].y};
        } else {
            vertex.TexCoord = {0.0f, 0.0f};
        }
        //if (aiMesh->HasTangentsAndBitangents()) {
        //    vertex.Tangent = {aiMesh->mTangents[v].x, aiMesh->mTangents[v].y, aiMesh->mTangents[v].z};
        //    vertex.BiTangent = {aiMesh->mBitangents[v].x, aiMesh->mBitangents[v].y, aiMesh->mBitangents[v].z};
        //} else {
        //    vertex.Tangent = {1.0f, 0.0f, 0.0f};
        //    vertex.BiTangent = {0.0f, 1.0f, 0.0f};
        //}

        vertices.push_back(vertex);
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

} // namespace MeshBaker
