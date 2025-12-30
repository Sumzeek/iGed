module;
#include "iGeMacro.h"

export module iGe.RHI:RHISemaphore;
import :RHIResource;
import iGe.Common;

namespace iGe
{

export class IGE_API RHISemaphore : public RHIResource {
public:
    ~RHISemaphore() override = default;

protected:
    RHISemaphore() : RHIResource(RHIResourceType::Semaphore) {}
};

} // namespace iGe
