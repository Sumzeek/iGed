module;
#include "iGeMacro.h"

export module iGe.RHI:RHIQueue;
import :RHIResource;
import :RHISemaphore;
import :RHIFence;
import iGe.Common;

namespace iGe
{

export class RHICommandList;

export enum class RHIQueueType : uint32 {
    Graphics = 0,
    Compute,
    Transfer,

    Count,
};

export struct RHIQueueCreateInfo {
    RHIQueueType Type;
    uint32 Index;
};

export class IGE_API RHIQueue : public RHIResource {
public:
    ~RHIQueue() override = default;

    inline RHIQueueType GetQueueType() const { return m_QueueType; }

    virtual void WaitIdle() = 0;

    virtual void Submit(const RHICommandList* commandList, RHIFence* fence = nullptr,
                        std::span<RHISemaphore*> waitSemaphores = {},
                        std::span<RHISemaphore*> signalSemaphores = {}) = 0;

protected:
    RHIQueue(const RHIQueueCreateInfo& info) : RHIResource(RHIResourceType::Queue), m_QueueType(info.Type) {}

    RHIQueueType m_QueueType;
};

} // namespace iGe
