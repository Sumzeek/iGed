module;
#include "iGeMacro.h"

export module iGe.RHI:RHIFramebuffer;
import :RHIResource;
import :RHIRenderPass;
import :RHITextureView;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// Framebuffer Create Info
// =================================================================================================

export struct RHIFramebufferCreateInfo {
    const RHIRenderPass* pRenderPass = nullptr;

    // Attachments (texture views, NOT textures - views define how to interpret texture memory)
    std::span<const RHITextureView* const> Attachments = {};

    // Dimensions
    uint32 Width = 0;
    uint32 Height = 0;
    uint32 Layers = 1; // For layered rendering (cubemaps, arrays, etc.)
};

// =================================================================================================
// Framebuffer Class
// =================================================================================================

export class IGE_API RHIFramebuffer : public RHIResource {
public:
    virtual ~RHIFramebuffer() override = default;

    uint32 GetWidth() const { return m_Width; }
    uint32 GetHeight() const { return m_Height; }
    uint32 GetLayers() const { return m_Layers; }
    uint32 GetAttachmentCount() const { return m_AttachmentCount; }

protected:
    RHIFramebuffer(const RHIFramebufferCreateInfo& info)
        : RHIResource(RHIResourceType::Framebuffer), m_Width(info.Width), m_Height(info.Height), m_Layers(info.Layers),
          m_AttachmentCount(static_cast<uint32>(info.Attachments.size())) {}

    uint32 m_Width = 0;
    uint32 m_Height = 0;
    uint32 m_Layers = 1;
    uint32 m_AttachmentCount = 0;
};

} // namespace iGe
