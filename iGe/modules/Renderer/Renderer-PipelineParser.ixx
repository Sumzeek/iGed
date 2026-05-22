module;
#include "iGeMacro.h"

export module iGe.Renderer:PipelineParser;
import iGe.RHI;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// ShaderLoader Type Definition
// =================================================================================================

export using ShaderLoader = std::function<Scope<RHIShader>(
        RHIShaderStage stage, const std::string& shaderName, const std::string& entryPoint)>;

// =================================================================================================
// PipelineParser
// =================================================================================================

export class IGE_API PipelineParser {
public:
    static Scope<RHIGraphicsPipeline> CreateGraphicsPipeline(const std::filesystem::path& jsonPath,
                                                             ShaderLoader shaderLoader,
                                                             const RHIRenderPass* pRenderPass = nullptr,
                                                             const RHIPipelineLayout* pPipelineLayout = nullptr);
};

} // namespace iGe
