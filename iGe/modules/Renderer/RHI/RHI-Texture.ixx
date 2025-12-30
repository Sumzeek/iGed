module;
#include "iGeMacro.h"

export module iGe.RHI:RHITexture;
import :RHIResource;
import iGe.Common;

namespace iGe
{

export struct RHIOffset2D {
    int32 X;
    int32 Y;
};

export struct RHIExtent2D {
    uint32 Width;
    uint32 Height;
};

export struct RHIExtent3D {
    uint32 Width;
    uint32 Height;
    uint32 Depth;
};

export enum class RHIFormat : uint32 {
    Unknown = 0,

    R8Srgb,
    R8G8Srgb,
    R8G8B8Srgb,
    R8G8B8A8Srgb,
    B8G8R8A8Srgb,

    R16SFloat,
    R16G16SFloat,
    R16G16B16SFloat,
    R16G16B16A16SFloat,
    R32SFloat,
    R32G32SFloat,
    R32G32B32SFloat,
    R32G32B32A32SFloat,

    R8UNorm,
    R8G8UNorm,
    R8G8B8UNorm,
    R8G8B8A8UNorm,
    R16UNorm,
    R16G16UNorm,
    R16G16B16UNorm,
    R16G16B16A16UNorm,

    R8SNorm,
    R8G8SNorm,
    R8G8B8SNorm,
    R8G8B8A8SNorm,
    R16SNorm,
    R16G16SNorm,
    R16G16B16SNorm,
    R16G16B16A16SNorm,

    R8UInt,
    R8G8UInt,
    R8G8B8UInt,
    R8G8B8A8UInt,
    R16UInt,
    R16G16UInt,
    R16G16B16UInt,
    R16G16B16A16UInt,
    R32UInt,
    R32G32UInt,
    R32G32B32UInt,
    R32G32B32A32UInt,

    R8SInt,
    R8G8SInt,
    R8G8B8SInt,
    R8G8B8A8SInt,
    R16SInt,
    R16G16SInt,
    R16G16B16SInt,
    R16G16B16A16SInt,
    R32SInt,
    R32G32SInt,
    R32G32B32SInt,
    R32G32B32A32SInt,

    D32SFloat,
    D32SFloatS8UInt,
    D24UNormS8UInt,

    Count
};

// =================================================================================================
// Texture Type
// =================================================================================================

export enum class RHITextureType : uint32 {
    Texture1D = 0,
    Texture2D,
    Texture3D,

    Count
};

// =================================================================================================
// Texture Usage Flags
// =================================================================================================

export enum class RHITextureUsageFlagBits : uint32 {
    None = 0,
    TransferSrc = 1 << 0,
    TransferDst = 1 << 1,
    Sampled = 1 << 2,
    Storage = 1 << 3,
    ColorAttachment = 1 << 4,
    DepthStencilAttachment = 1 << 5,
    TransientAttachment = 1 << 6,
    InputAttachment = 1 << 7,

    // Common combinations
    RenderTarget = ColorAttachment | Sampled,
    DepthStencil = DepthStencilAttachment | Sampled,
};

// =================================================================================================
// Sample Count Flags
// =================================================================================================

export enum class RHISampleCountFlagBits : uint32 {
    Count1 = 1 << 0,
    Count2 = 1 << 1,
    Count4 = 1 << 2,
    Count8 = 1 << 3,
    Count16 = 1 << 4,
    Count32 = 1 << 5,
    Count64 = 1 << 6,
};

// =================================================================================================
// Texture Create Info
// =================================================================================================

export struct RHITextureCreateInfo {
    RHITextureType Type = RHITextureType::Texture2D;
    RHIFormat Format = RHIFormat::R8G8B8A8Srgb;
    RHIExtent3D Extent = {1, 1, 1};

    uint32 MipLevels = 1;
    uint32 ArrayLayers = 1;
    RHISampleCountFlagBits Samples = RHISampleCountFlagBits::Count1;

    Flags<RHITextureUsageFlagBits> Usage = RHITextureUsageFlagBits::Sampled;
    RHIMemoryUsage MemoryUsage = RHIMemoryUsage::GpuOnly;

    // Initial data (optional, for immutable textures)
    const void* pInitialData = nullptr;
    uint64 InitialDataSize = 0;
};

// =================================================================================================
// Texture Class
// =================================================================================================

export class IGE_API RHITexture : public RHIResource {
public:
    ~RHITexture() override = default;

    RHITextureType GetType() const { return m_Type; }
    RHIFormat GetFormat() const { return m_Format; }
    const RHIExtent3D& GetExtent() const { return m_Extent; }
    uint32 GetWidth() const { return m_Extent.Width; }
    uint32 GetHeight() const { return m_Extent.Height; }
    uint32 GetDepth() const { return m_Extent.Depth; }
    uint32 GetMipLevels() const { return m_MipLevels; }
    uint32 GetArrayLayers() const { return m_ArrayLayers; }
    RHISampleCountFlagBits GetSamples() const { return m_Samples; }
    Flags<RHITextureUsageFlagBits> GetUsage() const { return m_Usage; }

    // Helper for 2D extent
    RHIExtent2D GetExtent2D() const { return {m_Extent.Width, m_Extent.Height}; }

protected:
    RHITexture(const RHITextureCreateInfo& info)
        : RHIResource(RHIResourceType::Texture), m_Type(info.Type), m_Format(info.Format), m_Extent(info.Extent),
          m_MipLevels(info.MipLevels), m_ArrayLayers(info.ArrayLayers), m_Samples(info.Samples), m_Usage(info.Usage),
          m_MemoryUsage(info.MemoryUsage) {}

    RHITextureType m_Type;
    RHIFormat m_Format;
    RHIExtent3D m_Extent;
    uint32 m_MipLevels;
    uint32 m_ArrayLayers;
    RHISampleCountFlagBits m_Samples;
    Flags<RHITextureUsageFlagBits> m_Usage;
    RHIMemoryUsage m_MemoryUsage;
};

} // namespace iGe
