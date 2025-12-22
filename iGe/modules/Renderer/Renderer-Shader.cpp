module;
#include "iGeMacro.h"
#include <nlohmann/json.hpp>

module iGe.Renderer;
import :Shader;
import :OpenGLShader;
import std;
import iGe.Log;

namespace iGe
{
ShaderStage ShaderStageFromString(const std::string& stageStr) {
    if (stageStr == "vertex") { return ShaderStage::Vertex; }
    if (stageStr == "tesscontrol" || stageStr == "hull") { return ShaderStage::TessellationControl; }
    if (stageStr == "tesseval" || stageStr == "domain") { return ShaderStage::TessellationEvaluation; }
    if (stageStr == "geometry") { return ShaderStage::Geometry; }
    if (stageStr == "fragment" || stageStr == "pixel") { return ShaderStage::Fragment; }
    if (stageStr == "compute") { return ShaderStage::Compute; }
    if (stageStr == "amplification") { return ShaderStage::Amplification; }
    if (stageStr == "mesh") { return ShaderStage::Mesh; }

    IGE_CORE_ASSERT(false, "Unknown file stage string!");
    return ShaderStage::None;
}

std::unordered_map<ShaderStage, std::filesystem::path> ParseShaderEntryMap(const std::filesystem::path& jsonFilePath) {
    std::ifstream inFile(jsonFilePath);
    if (!inFile) {
        IGE_CORE_WARN("Failed to open JSON file: {}", jsonFilePath.string());
        return {};
    }

    nlohmann::json jsonData;
    inFile >> jsonData;

    auto parentDir = jsonFilePath.parent_path();
    std::unordered_map<ShaderStage, std::filesystem::path> result;

    for (auto it = jsonData.begin(); it != jsonData.end(); ++it) {
        ShaderStage stage = ShaderStageFromString(it.key());
        std::filesystem::path relativePath = it.value().get<std::string>();
        result[stage] = parentDir / relativePath;
    }

    return result;
}

/////////////////////////////////////////////////////////////////////////////
// GraphicsShader ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Ref<GraphicsShader> GraphicsShader::Create(const std::filesystem::path& filepath) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLGraphicsShader>(filepath);
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

Ref<GraphicsShader> GraphicsShader::Create(const std::string& name, const std::filesystem::path& filepath) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLGraphicsShader>(name, filepath);
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

/////////////////////////////////////////////////////////////////////////////
// ComputeShader ////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Ref<ComputeShader> ComputeShader::Create(const std::filesystem::path& filepath) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLComputeShader>(filepath);
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

Ref<ComputeShader> ComputeShader::Create(const std::string& name, const std::filesystem::path& filepath) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLComputeShader>(name, filepath);
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

/////////////////////////////////////////////////////////////////////////////
// MeshShader ///////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Ref<MeshShader> MeshShader::Create(const std::filesystem::path& filepath) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLMeshShader>(filepath);
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

Ref<MeshShader> MeshShader::Create(const std::string& name, const std::filesystem::path& filepath) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLMeshShader>(name, filepath);
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

} // namespace iGe
