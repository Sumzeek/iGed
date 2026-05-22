module;
#include "iGeMacro.h"

export module iGe.Renderer:ShaderPackage;
import iGe.RHI;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// ShaderStageInfo
// =================================================================================================

export struct ShaderStageInfo {
    std::string Stage;
    std::string Entry;
    std::map<std::string, std::string> BytecodeFiles; // platform -> filename
};

// =================================================================================================
// ShaderReflectionResource
// =================================================================================================

export struct ShaderReflectionMember {
    std::string Name;
    std::string Type;
    uint32 Offset = 0;
    uint32 Size = 0;
};

export struct ShaderReflectionResource {
    std::string Name;
    std::string Type;
    uint32 Binding = 0;
    std::vector<std::string> Stages;
    std::vector<ShaderReflectionMember> Members;
};

// =================================================================================================
// ShaderReflectionVertexInput
// =================================================================================================

export struct ShaderReflectionVertexInput {
    std::string Name;
    uint32 Location = 0;
    std::string Format;
    std::string Semantic;
};

// =================================================================================================
// ShaderPackage
// =================================================================================================

export class IGE_API ShaderPackage {
public:
    [[nodiscard]] static Scope<ShaderPackage> Load(const std::string& shaderName,
                                     const std::filesystem::path& basePath = "assets/shaders");

    const std::string& GetName() const { return m_Name; }
    const std::vector<ShaderStageInfo>& GetStages() const { return m_Stages; }
    const std::vector<ShaderReflectionResource>& GetResources() const { return m_Resources; }
    const std::vector<ShaderReflectionVertexInput>& GetVertexInputs() const { return m_VertexInputs; }

    // Load bytecode for a specific stage and platform
    std::vector<uint8> LoadBytecode(RHIShaderStage stage) const;

    // Get platform key for current RHI backend
    static std::string GetPlatformKey();

private:
    std::string m_Name;
    std::filesystem::path m_PackageDir;
    std::vector<ShaderStageInfo> m_Stages;
    std::vector<ShaderReflectionResource> m_Resources;
    std::vector<ShaderReflectionVertexInput> m_VertexInputs;
};

} // namespace iGe
