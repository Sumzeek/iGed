module;
#include "iGeMacro.h"

export module iGe.RHI:RHIShader;
import :RHIResource;
import iGe.Common;

namespace iGe
{

export enum class RHIShaderStage : uint32 {
    Vertex = 0,
    Fragment,
    Geometry,
    TessControl,
    TessEvaluation,

    Compute,

    // RayTracing,

    // Amplification,
    // Mesh,

    Count
};

export struct RHIShaderCreateInfo {
    RHIShaderStage Stage;
    std::string EntryPoint = "main";
    std::string SourceCode;
};

export class IGE_API RHIShader : public RHIResource {
public:
    ~RHIShader() override = default;

    RHIShaderStage GetStage() const { return m_Stage; }
    std::string GetEntryPoint() const { return m_EntryPoint; }
    std::string GetSourceCode() const { return m_SourceCode; }

protected:
    RHIShader(const RHIShaderCreateInfo& info)
        : RHIResource(RHIResourceType::Shader), m_Stage(info.Stage), m_EntryPoint(info.EntryPoint),
          m_SourceCode(info.SourceCode) {}

    RHIShaderStage m_Stage;
    std::string m_EntryPoint = "main";
    std::string m_SourceCode;
};

} // namespace iGe
