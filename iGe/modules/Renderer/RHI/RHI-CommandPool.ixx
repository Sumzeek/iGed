module;
#include "iGeMacro.h"

export module iGe.RHI:RHICommandPool;
import :RHIResource;
import :RHIQueue;
import iGe.Common;

namespace iGe
{

export enum class RHICommandPoolCreateFlagBits : uint32 {
    None = 0,
    Transient = 1 << 0,          // Command buffers are short-lived
    ResetCommandBuffer = 1 << 1, // Command buffers can be reset individually
};

export struct RHICommandPoolCreateInfo {
    const RHIQueue* pQueue = nullptr;
    Flags<RHICommandPoolCreateFlagBits> Flags = RHICommandPoolCreateFlagBits::ResetCommandBuffer;
};

export class IGE_API RHICommandPool : public RHIResource {
public:
    ~RHICommandPool() override = default;

    // Reset the command allocator (all command lists must be closed first)
    virtual void Reset() = 0;

protected:
    RHICommandPool(const RHICommandPoolCreateInfo& info) : RHIResource(RHIResourceType::CommandPool) {}
};

} // namespace iGe
