module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"

export module iGe.RHI:DirectX12Surface;
import :RHISurface;
import iGe.Common;

namespace iGe
{

export class IGE_API DirectX12Surface : public RHISurface {
public:
    DirectX12Surface(const RHISurfaceCreateInfo& info) : RHISurface(info) {}
    ~DirectX12Surface() override = default;
};

} // namespace iGe
#endif
