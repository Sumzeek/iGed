module;
#include "iGeMacro.h"

export module iGe.RHI:RHIImGuiContext;
import :RHITexture;
import iGe.Common;

namespace iGe
{

export class IGE_API RHIImGuiContext {
public:
    struct Config {
        void* Window = nullptr;
        uint32 MaxFramesInFlight = 1;
    };

    virtual ~RHIImGuiContext() = default;

    static RHIImGuiContext* Init(const Config& config);
    static RHIImGuiContext* Get() { return s_ImGuiContext.get(); }

    virtual void Begin(uint32 frameIndex = 0) = 0;
    virtual void End() = 0;
    virtual void SetRenderTarget(RHITexture& target) = 0;

protected:
    RHIImGuiContext() = default;

    inline static Config s_Config{};
    inline static Scope<RHIImGuiContext> s_ImGuiContext = nullptr;
};

} // namespace iGe
