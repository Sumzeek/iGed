module;
#include <assimp/Exporter.hpp>
#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

module MeshBaker;
import :Mesh;

namespace MeshBaker
{

Mesh LoadObjFile(const std::filesystem::path& filepath) {
    Mesh mesh;
    mesh.Name = filepath.stem().string();

    // read file via assimp
    Assimp::Importer importer;
    const aiScene* scene =
            importer.ReadFile(filepath.string(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
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

static aiScene* ConvertMeshToAssimpScene(const Mesh& mesh) {
    aiScene* scene = new aiScene();
    scene->mRootNode = new aiNode();

    aiMesh* ai_mesh = new aiMesh();
    ai_mesh->mMaterialIndex = 0;
    ai_mesh->mNumVertices = (unsigned int) mesh.Vertices.size();
    ai_mesh->mVertices = new aiVector3D[ai_mesh->mNumVertices];
    ai_mesh->mNormals = new aiVector3D[ai_mesh->mNumVertices];
    ai_mesh->mTextureCoords[0] = new aiVector3D[ai_mesh->mNumVertices];

    for (size_t i = 0; i < mesh.Vertices.size(); i++) {
        const Vertex& v = mesh.Vertices[i];
        ai_mesh->mVertices[i] = aiVector3D(v.Position.x, v.Position.y, v.Position.z);
        ai_mesh->mNormals[i] = aiVector3D(v.Normal.x, v.Normal.y, v.Normal.z);
        ai_mesh->mTextureCoords[0][i] = aiVector3D(v.TexCoord.x, v.TexCoord.y, 0.0f);
    }

    size_t faceCount = mesh.Indices.size() / 3;
    ai_mesh->mNumFaces = (unsigned int) faceCount;
    ai_mesh->mFaces = new aiFace[ai_mesh->mNumFaces];

    for (size_t i = 0; i < faceCount; i++) {
        aiFace& face = ai_mesh->mFaces[i];
        face.mNumIndices = 3;
        face.mIndices = new unsigned int[3];
        face.mIndices[0] = mesh.Indices[i * 3 + 0];
        face.mIndices[1] = mesh.Indices[i * 3 + 1];
        face.mIndices[2] = mesh.Indices[i * 3 + 2];
    }

    scene->mNumMeshes = 1;
    scene->mMeshes = new aiMesh*[1];
    scene->mMeshes[0] = ai_mesh;

    scene->mRootNode->mNumMeshes = 1;
    scene->mRootNode->mMeshes = new unsigned int[1];
    scene->mRootNode->mMeshes[0] = 0;

    scene->mNumMaterials = 1;
    scene->mMaterials = new aiMaterial*[1];
    scene->mMaterials[0] = new aiMaterial();

    return scene;
}
void ExportMeshAsOBJ(const Mesh& mesh) {
    std::string filepath = "assets/models/" + mesh.Name + ".obj";

    Assimp::Exporter exporter;
    aiScene* scene = ConvertMeshToAssimpScene(mesh);

    aiReturn ret = exporter.Export(scene, "obj", filepath);
    if (ret != aiReturn_SUCCESS) { std::cerr << "Assimp export failed: " << exporter.GetErrorString() << std::endl; }

    delete scene;
}
} // namespace MeshBaker
