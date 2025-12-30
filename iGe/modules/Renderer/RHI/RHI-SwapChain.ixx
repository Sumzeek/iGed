module;
#include "iGeMacro.h"

export module iGe.RHI:RHISwapChain;
import :RHIResource;
import :RHISurface;
import :RHIQueue;
import :RHITexture;
import :RHITextureView;
import iGe.Common;
import std;

namespace iGe
{

export struct RHISwapChainCreateInfo {
    const RHISurface* Surface = nullptr;
    const RHIQueue* PresentQueue = nullptr;

    uint32 ImageCount = 1;
    RHIExtent2D Extent;
    RHIFormat Format = RHIFormat::B8G8R8A8Srgb;
};

export class IGE_API RHISwapChain : public RHIResource {
public:
    ~RHISwapChain() override = default;

    // Acquire the next image index for rendering
    virtual uint32 AcquireNextImage(RHISemaphore* signalSemaphore = nullptr, RHIFence* signalFence = nullptr) = 0;

    // Present the current back buffer
    virtual void Present(std::span<RHISemaphore* const> waitSemaphores = {}) = 0;

    // Resize swap chain
    virtual void Resize(uint32 width, uint32 height) = 0;

    // Get back buffer texture at index
    virtual RHITexture* GetBackBufferTexture(uint32 index) const = 0;

    // Get back buffer texture view at index (for rendering)
    virtual RHITextureView* GetBackBufferView(uint32 index) const = 0;

    // Getters for swap chain properties
    uint32 GetImageCount() const { return m_ImageCount; }
    uint32 GetWidth() const { return m_Extent.Width; }
    uint32 GetHeight() const { return m_Extent.Height; }
    RHIFormat GetFormat() const { return m_Format; }

protected:
    RHISwapChain(const RHISwapChainCreateInfo& info)
        : RHIResource(RHIResourceType::SwapChain), m_ImageCount(info.ImageCount), m_Extent(info.Extent),
          m_Format(info.Format) {}

    uint32 m_ImageCount;
    RHIExtent2D m_Extent;
    RHIFormat m_Format;
};

} // namespace iGe
