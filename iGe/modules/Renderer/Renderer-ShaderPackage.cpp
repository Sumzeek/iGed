module;
#include <fstream>
#include <nlohmann/json.hpp>

module iGe.Renderer;
import :ShaderPackage;

namespace iGe
{

// =================================================================================================
// Helper: RHIShaderStage to string
// =================================================================================================

static std::string ShaderStageToString(RHIShaderStage stage) {
    switch (stage) {
        case RHIShaderStage::Vertex: return "vertex";
        case RHIShaderStage::Fragment: return "fragment";
        case RHIShaderStage::Geometry: return "geometry";
        case RHIShaderStage::TessControl: return "tesscontrol";
        case RHIShaderStage::TessEvaluation: return "tesseval";
        case RHIShaderStage::Compute: return "compute";
        default: return "unknown";
    }
}

// =================================================================================================
// ShaderPackage Implementation
// =================================================================================================

std::string ShaderPackage::GetPlatformKey() {
    switch (RHI::GetGraphicsAPI()) {
        case GraphicsAPI::DirectX12: return "dxil";
        case GraphicsAPI::Vulkan: return "spirv";
        default:
            Internal::LogError("ShaderPackage: Unsupported graphics API");
            return "dxil";
    }
}

Scope<ShaderPackage> ShaderPackage::Load(const std::string& shaderName,
                                          const std::filesystem::path& basePath) {
    auto packageDir = basePath / shaderName;
    auto jsonPath = packageDir / (shaderName + ".shader.json");

    if (!std::filesystem::exists(jsonPath)) {
        Internal::LogError("ShaderPackage: Not found - {}", jsonPath.string());
        return nullptr;
    }

    std::ifstream file(jsonPath);
    if (!file.is_open()) {
        Internal::LogError("ShaderPackage: Failed to open - {}", jsonPath.string());
        return nullptr;
    }

    try {
        nlohmann::json j = nlohmann::json::parse(file);

        auto pkg = CreateScope<ShaderPackage>();
        pkg->m_Name = j.value("name", shaderName);
        pkg->m_PackageDir = packageDir;

        // Parse stages
        if (j.contains("stages") && j["stages"].is_array()) {
            for (const auto& stageJson : j["stages"]) {
                ShaderStageInfo info;
                info.Stage = stageJson.value("stage", "");
                info.Entry = stageJson.value("entry", "main");

                if (stageJson.contains("bytecode") && stageJson["bytecode"].is_object()) {
                    for (auto& [key, val] : stageJson["bytecode"].items()) {
                        info.BytecodeFiles[key] = val.get<std::string>();
                    }
                }

                pkg->m_Stages.push_back(std::move(info));
            }
        }

        // Parse reflection
        if (j.contains("reflection")) {
            const auto& ref = j["reflection"];

            // Resources
            if (ref.contains("resources") && ref["resources"].is_array()) {
                for (const auto& resJson : ref["resources"]) {
                    ShaderReflectionResource res;
                    res.Name = resJson.value("name", "");
                    res.Type = resJson.value("type", "");
                    res.Binding = resJson.value("binding", 0u);

                    if (resJson.contains("stages") && resJson["stages"].is_array()) {
                        for (const auto& s : resJson["stages"]) {
                            res.Stages.push_back(s.get<std::string>());
                        }
                    }

                    if (resJson.contains("members") && resJson["members"].is_array()) {
                        for (const auto& memJson : resJson["members"]) {
                            ShaderReflectionMember mem;
                            mem.Name = memJson.value("name", "");
                            mem.Type = memJson.value("type", "");
                            mem.Offset = memJson.value("offset", 0u);
                            mem.Size = memJson.value("size", 0u);
                            res.Members.push_back(std::move(mem));
                        }
                    }

                    pkg->m_Resources.push_back(std::move(res));
                }
            }

            // Vertex inputs
            if (ref.contains("vertexInputs") && ref["vertexInputs"].is_array()) {
                for (const auto& viJson : ref["vertexInputs"]) {
                    ShaderReflectionVertexInput vi;
                    vi.Name = viJson.value("name", "");
                    vi.Location = viJson.value("location", 0u);
                    vi.Format = viJson.value("format", "");
                    vi.Semantic = viJson.value("semantic", "");
                    pkg->m_VertexInputs.push_back(std::move(vi));
                }
            }
        }

        return pkg;

    } catch (const nlohmann::json::exception& e) {
        Internal::LogError("ShaderPackage: JSON parse error - {}", e.what());
        return nullptr;
    }
}

std::vector<uint8> ShaderPackage::LoadBytecode(RHIShaderStage stage) const {
    std::string stageStr = ShaderStageToString(stage);
    std::string platformKey = GetPlatformKey();

    for (const auto& stageInfo : m_Stages) {
        if (stageInfo.Stage == stageStr) {
            auto it = stageInfo.BytecodeFiles.find(platformKey);
            if (it == stageInfo.BytecodeFiles.end()) {
                Internal::LogError("ShaderPackage: No bytecode for platform '{}' in shader '{}'",
                                   platformKey, m_Name);
                return {};
            }

            auto bytecodeFile = m_PackageDir / it->second;
            std::ifstream file(bytecodeFile, std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                Internal::LogError("ShaderPackage: Failed to open bytecode - {}",
                                   bytecodeFile.string());
                return {};
            }

            auto size = static_cast<size_t>(file.tellg());
            std::vector<uint8> bytecode(size);
            file.seekg(0);
            file.read(reinterpret_cast<char*>(bytecode.data()), size);
            return bytecode;
        }
    }

    Internal::LogError("ShaderPackage: Stage '{}' not found in shader '{}'", stageStr, m_Name);
    return {};
}

} // namespace iGe
