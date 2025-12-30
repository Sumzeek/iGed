module;
#include "iGeMacro.h"

export module iGe.RHI:RHIRenderPass;
import :RHIResource;
import :RHITexture;
import :RHITextureView;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// Enums
// =================================================================================================

export enum class RHIDependencyAccess : uint32 {
    None = 0,
    IndirectCommandRead = 1 << 0,
    IndexRead = 1 << 1,
    VertexAttributeRead = 1 << 2,
    UniformRead = 1 << 3,
    InputAttachmentRead = 1 << 4,
    ShaderRead = 1 << 5,
    ShaderWrite = 1 << 6,
    ColorAttachmentRead = 1 << 7,
    ColorAttachmentWrite = 1 << 8,
    DepthStencilAttachmentRead = 1 << 9,
    DepthStencilAttachmentWrite = 1 << 10,
    TransferRead = 1 << 11,
    TransferWrite = 1 << 12,
    HostRead = 1 << 13,
    HostWrite = 1 << 14,
    MemoryRead = 1 << 15,
    MemoryWrite = 1 << 16
};

export enum class RHILayout : uint32 {
    Undefined = 0,
    General,
    ColorAttachment,
    DepthStencilAttachment,
    DepthStencilReadOnly,
    ShaderReadOnly,
    TransferSrc,
    TransferDst,
    Preinitialized,
    Present,
    Common,
    ShadingRateSurface,
    FragmentDensityMap,

    Count
};

export enum class RHILoadOp : uint32 { Load = 0, Clear, DontCare, Count };

export enum class RHIStoreOp : uint32 { Store = 0, DontCare, Count };

// =================================================================================================
// Clear Value
// =================================================================================================

export struct RHIClearValue {
    union {
        float Color[4];
        struct {
            float Depth;
            uint32 Stencil;
        } DepthStencil;
    };

    RHIClearValue() : Color{0.0f, 0.0f, 0.0f, 1.0f} {}

    static RHIClearValue CreateColor(float r, float g, float b, float a) {
        RHIClearValue v{};
        v.Color[0] = r;
        v.Color[1] = g;
        v.Color[2] = b;
        v.Color[3] = a;
        return v;
    }

    static RHIClearValue CreateDepthStencil(float depth, uint32 stencil = 0) {
        RHIClearValue v{};
        v.DepthStencil.Depth = depth;
        v.DepthStencil.Stencil = stencil;
        return v;
    }
};

// =================================================================================================
// Attachment Description (for RenderPass creation)
// =================================================================================================

export struct RHIAttachmentDescription {
    RHIFormat Format = RHIFormat::Unknown;
    uint32 SampleCount = 1;

    RHILoadOp LoadOp = RHILoadOp::DontCare;
    RHIStoreOp StoreOp = RHIStoreOp::DontCare;

    RHILoadOp StencilLoadOp = RHILoadOp::DontCare;
    RHIStoreOp StencilStoreOp = RHIStoreOp::DontCare;

    RHILayout InitialLayout = RHILayout::Undefined;
    RHILayout FinalLayout = RHILayout::Undefined;
};

// =================================================================================================
// Subpass
// =================================================================================================

export struct RHISubpassDescription {
    std::span<const uint32> InputAttachments = {};
    std::span<const uint32> ColorAttachments = {};
    std::span<const uint32> ResolveAttachments = {}; // If not empty, must have same count as ColorAttachments
    uint32 DepthStencilAttachment = ~0u;
    std::span<const uint32> PreserveAttachments = {};

    bool HasDepthStencilAttachment() const { return DepthStencilAttachment != ~0u; }
};

export struct RHISubpassDependency {
    uint32 SrcSubpass = ~0u;
    uint32 DstSubpass = 0;
    Flags<RHIDependencyAccess> SrcAccessMask = RHIDependencyAccess::None;
    Flags<RHIDependencyAccess> DstAccessMask = RHIDependencyAccess::None;
};

// =================================================================================================
// RenderPass Create Info
// =================================================================================================

export struct RHIRenderPassCreateInfo {
    std::span<const RHIAttachmentDescription> Attachments = {};
    std::span<const RHISubpassDescription> Subpasses = {};
    std::span<const RHISubpassDependency> Dependencies = {};
};

// =================================================================================================
// RHIRenderPass
// =================================================================================================

export class IGE_API RHIRenderPass : public RHIResource {
public:
    ~RHIRenderPass() override = default;

protected:
    RHIRenderPass(const RHIRenderPassCreateInfo& info) : RHIResource(RHIResourceType::RenderPass) {}
};

// =================================================================================================
// Attachment Binding (for RenderPass Begin)
// =================================================================================================

// Represents a texture view bound as an attachment during render pass execution
export struct RHIAttachmentBinding {
    const RHITextureView* pTextureView;
    RHIClearValue ClearValue;
};

export struct RHIRenderPassBeginInfo {
    const RHIRenderPass* pRenderPass = nullptr;

    // Attachments bound in the same order as RenderPassCreateInfo::Attachments
    std::span<const RHIAttachmentBinding> ColorAttachments = {};
    const RHIAttachmentBinding* pDepthStencilAttachment = nullptr;

    // Render area
    RHIOffset2D RenderAreaOffset = {0, 0};
    RHIExtent2D RenderAreaExtent = {0, 0};
};

} // namespace iGe
