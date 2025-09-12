module;
#include <nlohmann/json.hpp>

module iGe.Renderer;
import :Shader;
import :Renderer;
import :OpenGLShader;

namespace iGe
{
ShaderStage ShaderStageFromString(const std::string& stageStr) {
    if (stageStr == "vertex") { return ShaderStage::Vertex; }
    if (stageStr == "tesscontrol" || stageStr == "hull") { return ShaderStage::TessellationControl; }
    if (stageStr == "tesseval" || stageStr == "domain") { return ShaderStage::TessellationEvaluation; }
    if (stageStr == "geometry") { return ShaderStage::Geometry; }
    if (stageStr == "fragment" || stageStr == "pixel") { return ShaderStage::Fragment; }
    if (stageStr == "compute") { return ShaderStage::Compute; }

    Internal::Assert(false, "Unknown file stage string!");
    return ShaderStage::None;
}

std::unordered_map<ShaderStage, std::filesystem::path> ParseShaderEntryMap(const std::filesystem::path& jsonFilePath) {
    std::ifstream inFile(jsonFilePath);
    if (!inFile) {
        Internal::LogWarn("Failed to open JSON file: {}", jsonFilePath.string());
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
// Shader ///////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Ref<Shader> Shader::Create(const std::filesystem::path& filepath) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            Internal::Assert(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLShader>(filepath);
        case RendererAPI::API::Vulkan:
            Internal::Assert(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    Internal::Assert(false, "Unknown RendererAPI!");
    return nullptr;
}

Ref<Shader> Shader::Create(const std::string& name, const std::filesystem::path& filepath) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            Internal::Assert(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLShader>(name, filepath);
        case RendererAPI::API::Vulkan:
            Internal::Assert(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    Internal::Assert(false, "Unknown RendererAPI!");
    return nullptr;
}

/////////////////////////////////////////////////////////////////////////////
// ShaderLibrary ////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void ShaderLibrary::Add(const std::string& name, const Ref<Shader>& shader) {
    Internal::Assert(!Exists(name), "Shader already exists!");
    m_Shaders[name] = shader;
}

void ShaderLibrary::Add(const Ref<Shader>& shader) {
    auto& name = shader->GetName();
    Internal::Assert(!Exists(name), "Shader already exists!");
    Add(name, shader);
}

Ref<Shader> ShaderLibrary::Load(const std::filesystem::path& filepath) {
    auto shader = Shader::Create(filepath);
    Add(shader);
    return shader;
}

Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::filesystem::path& filepath) {
    auto shader = Shader::Create(name, filepath);
    Add(name, shader);
    return shader;
}

Ref<Shader> ShaderLibrary::Get(const std::string& name) {
    Internal::Assert(Exists(name), "Shader not found!");
    return m_Shaders[name];
}

bool ShaderLibrary::Exists(const std::string& name) const { return m_Shaders.find(name) != m_Shaders.end(); }
} // namespace iGe
