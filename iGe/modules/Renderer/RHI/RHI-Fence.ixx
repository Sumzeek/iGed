module;
#include "iGeMacro.h"

export module iGe.RHI:RHIFence;
import :RHIResource;
import iGe.Common;

namespace iGe
{

export struct RHIFenceCreateInfo {
    bool Signaled = false;
    uint64 InitialValue = 0;
};

export class IGE_API RHIFence : public RHIResource {
public:
    ~RHIFence() override = default;

    virtual bool Wait(uint64 timeout = std::numeric_limits<uint64>::max()) = 0;
    virtual void Reset() = 0;

protected:
    RHIFence(const RHIFenceCreateInfo& info) : RHIResource(RHIResourceType::Fence) {}
};

} // namespace iGe
