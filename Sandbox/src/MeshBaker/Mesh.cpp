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
                                                         aiProcess_GenBoundingBoxes /*| aiProcess_ForceGenNormals |
                                                         aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace*/);
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

void FillCurvature(Mesh& mesh, int w, int h, const std::vector<float>& displacementValues) {
    auto sampleEdgeCurvature = [&](const glm::vec2& tA, const glm::vec2& tB, const glm::vec3& pA,
                                   const glm::vec3& pB) -> float {
        std::vector<float> heights;
        int xp = static_cast<int>(tB.x - tA.x);
        int yp = static_cast<int>(tB.y - tA.y);

        int ps = std::max(std::abs(xp), std::abs(yp));
        int dx = xp / ps;
        int dy = yp / ps;

        for (int s = 0; s <= ps; ++s) {
            int x = static_cast<int>(tA.x) + s * dx;
            int y = static_cast<int>(tA.y) + s * dy;

            x = std::clamp(x, 0, w - 1);
            y = std::clamp(y, 0, h - 1);

            heights.push_back(displacementValues[y * w + x]);
        }

        float curvature = 0.0f;
        if (heights.size() < 3) { return 0.0f; }

        float edgeLen = glm::length(pA - pB);
        if (edgeLen < 1e-6f) { return 0.0f; }

        float step = edgeLen / (heights.size() - 1);
        for (int j = 1; j < (int) heights.size() - 1; ++j) {
            float y_prev = heights[j - 1];
            float y_curr = heights[j];
            float y_next = heights[j + 1];

            float yd = (y_next - y_prev) / (2 * step);
            float ydd = (y_next - 2 * y_curr + y_prev) / (step * step);

            float k = ydd / std::pow(1.0f + yd * yd, 1.5f);
            curvature = std::max(curvature, std::abs(k));
        }

        return curvature;
    };

    for (int i = 0; i < mesh.Indices.size(); i += 3) {
        auto& v0 = mesh.Vertices[mesh.Indices[i + 0]];
        auto& v1 = mesh.Vertices[mesh.Indices[i + 1]];
        auto& v2 = mesh.Vertices[mesh.Indices[i + 2]];

        float k0 = sampleEdgeCurvature(v1.TexCoord, v2.TexCoord, v1.Position, v2.Position);
        float k1 = sampleEdgeCurvature(v2.TexCoord, v0.TexCoord, v2.Position, v0.Position);
        float k2 = sampleEdgeCurvature(v0.TexCoord, v1.TexCoord, v0.Position, v1.Position);

        v0.Curvature = k0;
        v1.Curvature = k1;
        v2.Curvature = k2;
    }
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
