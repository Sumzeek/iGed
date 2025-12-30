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
        RHIShaderStage stage, const std::filesystem::path& shaderPath, const std::string& entryPoint)>;

// =================================================================================================
// PipelineParser
// =================================================================================================

export class IGE_API PipelineParser {
public:
    static Scope<RHIGraphicsPipeline> CreateGraphicsPipeline(const std::filesystem::path& jsonContent,
                                                             ShaderLoader shaderLoader,
                                                             const RHIRenderPass* pRenderPass = nullptr,
                                                             const RHIPipelineLayout* pPipelineLayout = nullptr);
};

} // namespace iGe
