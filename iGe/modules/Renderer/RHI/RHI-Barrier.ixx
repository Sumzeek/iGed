module;
#include "iGeMacro.h"

export module iGe.RHI:RHIBarrier;
import :RHIBuffer;
import :RHITexture;
import :RHIRenderPass; // For RHILayout, RHIDependencyAccess
import iGe.Common;

namespace iGe
{

export enum class RHIPipelineStageFlagBits : uint32 {
    None = 0,
    TopOfPipe = 1 << 0,
    DrawIndirect = 1 << 1,
    VertexInput = 1 << 2,
    VertexShader = 1 << 3,
    TessellationControlShader = 1 << 4,
    TessellationEvaluationShader = 1 << 5,
    GeometryShader = 1 << 6,
    FragmentShader = 1 << 7,
    EarlyFragmentTests = 1 << 8,
    LateFragmentTests = 1 << 9,
    ColorAttachmentOutput = 1 << 10,
    ComputeShader = 1 << 11,
    Transfer = 1 << 12,
    BottomOfPipe = 1 << 13,
    Host = 1 << 14,
    AllGraphics = 1 << 15,
    AllCommands = 1 << 16,

    // Ray tracing stages (if supported)
    RayTracingShader = 1 << 17,
    AccelerationStructureBuild = 1 << 18,
};

// =================================================================================================
// Memory Barrier
// =================================================================================================

export struct RHIMemoryBarrier {
    Flags<RHIPipelineStageFlagBits> SrcStageMask = RHIPipelineStageFlagBits::None;
    Flags<RHIPipelineStageFlagBits> DstStageMask = RHIPipelineStageFlagBits::None;
    Flags<RHIDependencyAccess> SrcAccessMask = RHIDependencyAccess::None;
    Flags<RHIDependencyAccess> DstAccessMask = RHIDependencyAccess::None;
};

// =================================================================================================
// Buffer Memory Barrier
// =================================================================================================

export struct RHIBufferMemoryBarrier {
    const RHIBuffer* pBuffer = nullptr;
    uint64 Offset = 0;
    uint64 Size = ~0ULL; // VK_WHOLE_SIZE

    Flags<RHIPipelineStageFlagBits> SrcStageMask = RHIPipelineStageFlagBits::None;
    Flags<RHIPipelineStageFlagBits> DstStageMask = RHIPipelineStageFlagBits::None;
    Flags<RHIDependencyAccess> SrcAccessMask = RHIDependencyAccess::None;
    Flags<RHIDependencyAccess> DstAccessMask = RHIDependencyAccess::None;

    // For queue family ownership transfer
    uint32 SrcQueueFamilyIndex = ~0u; // VK_QUEUE_FAMILY_IGNORED
    uint32 DstQueueFamilyIndex = ~0u;
};

// =================================================================================================
// Image/Texture Memory Barrier
// =================================================================================================

export struct RHITextureMemoryBarrier {
    const RHITexture* pTexture = nullptr;

    RHILayout OldLayout = RHILayout::Undefined;
    RHILayout NewLayout = RHILayout::General;

    Flags<RHIPipelineStageFlagBits> SrcStageMask = RHIPipelineStageFlagBits::None;
    Flags<RHIPipelineStageFlagBits> DstStageMask = RHIPipelineStageFlagBits::None;
    Flags<RHIDependencyAccess> SrcAccessMask = RHIDependencyAccess::None;
    Flags<RHIDependencyAccess> DstAccessMask = RHIDependencyAccess::None;

    // For queue family ownership transfer
    uint32 SrcQueueFamilyIndex = ~0u;
    uint32 DstQueueFamilyIndex = ~0u;
};

// =================================================================================================
// Barrier Batch (for submitting multiple barriers at once)
// =================================================================================================

export struct RHIBarrierBatch {
    std::span<const RHIMemoryBarrier> MemoryBarriers = {};
    std::span<const RHIBufferMemoryBarrier> BufferBarriers = {};
    std::span<const RHITextureMemoryBarrier> TextureBarriers = {};

    // Dependency flags
    bool ByRegion = false; // VK_DEPENDENCY_BY_REGION_BIT
};

// =================================================================================================
// Helper functions to create common barriers
// =================================================================================================

// export namespace RHIBarrierHelper
// {
//
// // Transition texture for shader read
// inline RHITextureMemoryBarrier TransitionToShaderRead(RHITexture* texture, RHILayout oldLayout) {
//     RHITextureMemoryBarrier barrier{};
//     barrier.Texture = texture;
//     barrier.OldLayout = oldLayout;
//     barrier.NewLayout = RHILayout::ShaderReadOnly;
//     barrier.SrcStageMask = RHIPipelineStageFlagBits::AllCommands;
//     barrier.DstStageMask = RHIPipelineStageFlagBits::FragmentShader;
//     barrier.SrcAccessMask = RHIDependencyAccess::MemoryWrite;
//     barrier.DstAccessMask = RHIDependencyAccess::ShaderRead;
//     return barrier;
// }
//
// // Transition texture for color attachment
// inline RHITextureMemoryBarrier TransitionToColorAttachment(RHITexture* texture, RHILayout oldLayout) {
//     RHITextureMemoryBarrier barrier{};
//     barrier.Texture = texture;
//     barrier.OldLayout = oldLayout;
//     barrier.NewLayout = RHILayout::ColorAttachment;
//     barrier.SrcStageMask = RHIPipelineStageFlagBits::AllCommands;
//     barrier.DstStageMask = RHIPipelineStageFlagBits::ColorAttachmentOutput;
//     barrier.SrcAccessMask = RHIDependencyAccess::MemoryRead;
//     barrier.DstAccessMask = RHIDependencyAccess::ColorAttachmentWrite;
//     return barrier;
// }
//
// // Transition texture for depth attachment
// inline RHITextureMemoryBarrier TransitionToDepthAttachment(RHITexture* texture, RHILayout oldLayout) {
//     RHITextureMemoryBarrier barrier{};
//     barrier.Texture = texture;
//     barrier.OldLayout = oldLayout;
//     barrier.NewLayout = RHILayout::DepthStencilAttachment;
//     barrier.SrcStageMask = RHIPipelineStageFlagBits::AllCommands;
//     barrier.DstStageMask = RHIPipelineStageFlagBits::EarlyFragmentTests | RHIPipelineStageFlagBits::LateFragmentTests;
//     barrier.SrcAccessMask = RHIDependencyAccess::MemoryRead;
//     barrier.DstAccessMask = RHIDependencyAccess::DepthStencilAttachmentWrite;
//     return barrier;
// }
//
// // Transition texture for transfer source
// inline RHITextureMemoryBarrier TransitionToTransferSrc(RHITexture* texture, RHILayout oldLayout) {
//     RHITextureMemoryBarrier barrier{};
//     barrier.Texture = texture;
//     barrier.OldLayout = oldLayout;
//     barrier.NewLayout = RHILayout::TransferSrc;
//     barrier.SrcStageMask = RHIPipelineStageFlagBits::AllCommands;
//     barrier.DstStageMask = RHIPipelineStageFlagBits::Transfer;
//     barrier.SrcAccessMask = RHIDependencyAccess::MemoryWrite;
//     barrier.DstAccessMask = RHIDependencyAccess::TransferRead;
//     return barrier;
// }
//
// // Transition texture for transfer destination
// inline RHITextureMemoryBarrier TransitionToTransferDst(RHITexture* texture, RHILayout oldLayout) {
//     RHITextureMemoryBarrier barrier{};
//     barrier.Texture = texture;
//     barrier.OldLayout = oldLayout;
//     barrier.NewLayout = RHILayout::TransferDst;
//     barrier.SrcStageMask = RHIPipelineStageFlagBits::AllCommands;
//     barrier.DstStageMask = RHIPipelineStageFlagBits::Transfer;
//     barrier.SrcAccessMask = RHIDependencyAccess::MemoryRead;
//     barrier.DstAccessMask = RHIDependencyAccess::TransferWrite;
//     return barrier;
// }
//
// // Transition texture for present
// inline RHITextureMemoryBarrier TransitionToPresent(RHITexture* texture, RHILayout oldLayout) {
//     RHITextureMemoryBarrier barrier{};
//     barrier.Texture = texture;
//     barrier.OldLayout = oldLayout;
//     barrier.NewLayout = RHILayout::Present;
//     barrier.SrcStageMask = RHIPipelineStageFlagBits::ColorAttachmentOutput;
//     barrier.DstStageMask = RHIPipelineStageFlagBits::BottomOfPipe;
//     barrier.SrcAccessMask = RHIDependencyAccess::ColorAttachmentWrite;
//     barrier.DstAccessMask = RHIDependencyAccess::None;
//     return barrier;
// }
//
// } // namespace RHIBarrierHelper

} // namespace iGe
