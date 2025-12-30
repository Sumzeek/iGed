module;
#include "iGeMacro.h"

export module iGe.RHI:RHISurface;
import :RHIResource;
import iGe.Common;

namespace iGe
{

export struct RHISurfaceCreateInfo {
    void* WindowHandle = nullptr;
};

export class IGE_API RHISurface : public RHIResource {
public:
    ~RHISurface() override = default;

    void* GetNativeWindowHandle() const { return m_WindowHandle; }

protected:
    RHISurface(const RHISurfaceCreateInfo& info)
        : RHIResource(RHIResourceType::Surface), m_WindowHandle(info.WindowHandle) {};

    void* m_WindowHandle;
};

} // namespace iGe
