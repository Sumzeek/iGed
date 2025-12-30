module;
#include "iGeMacro.h"

export module iGe.RHI:RHIComputePipeline;
import :RHIResource;
import :RHIShader;
import :RHIDescriptor;       // For RHIPipelineLayout complete type
import :RHIGraphicsPipeline; // For RHISpecializationInfo
import iGe.Common;

namespace iGe
{

// =================================================================================================
// Compute Pipeline Create Info
// =================================================================================================

export class RHIComputePipeline;
export struct RHIComputePipelineCreateInfo {
    // Required: Compute shader
    const RHIShader* pComputeShader = nullptr;

    // Pipeline layout (defines descriptor set layouts and push constants)
    const RHIPipelineLayout* pLayout = nullptr;

    // Optional: Specialization constants
    RHISpecializationInfo SpecializationInfo;

    // Optional: Base pipeline for derivatives (can improve creation time)
    const RHIComputePipeline* pBasePipeline = nullptr;
    int32 BasePipelineIndex = -1;
};

// =================================================================================================
// Compute Pipeline
// =================================================================================================

export class IGE_API RHIComputePipeline : public RHIResource {
public:
    ~RHIComputePipeline() override = default;

protected:
    RHIComputePipeline(const RHIComputePipelineCreateInfo& info) : RHIResource(RHIResourceType::ComputePipeline) {}
};

} // namespace iGe
