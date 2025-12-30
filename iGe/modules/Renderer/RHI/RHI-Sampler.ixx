module;
#include "iGeMacro.h"

export module iGe.RHI:RHISampler;
import :RHIResource;
import iGe.Common;

namespace iGe
{

export enum class RHISamplerFilter : uint32 {
    Nearest = 0,
    Linear,
    CubicImg, // VK_FILTER_CUBIC_IMG

    Count
};

export enum class RHISamplerMipmapMode : uint32 {
    Nearest = 0,
    Linear,

    Count
};

export enum class RHISamplerAddressMode : uint32 {
    Repeat = 0,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder,
    MirrorClampToEdge,

    Count
};

export enum class RHIBorderColor : uint32 {
    FloatTransparentBlack = 0,
    IntTransparentBlack,
    FloatOpaqueBlack,
    IntOpaqueBlack,
    FloatOpaqueWhite,
    IntOpaqueWhite,

    Count
};

export enum class RHICompareOp : uint32 {
    Never = 0,
    Less,
    Equal,
    LessOrEqual,
    Greater,
    NotEqual,
    GreaterOrEqual,
    Always,

    Count
};

export enum class RHIStencilOp : uint32 {
    Keep = 0,
    Zero,
    Replace,
    IncrementAndClamp,
    DecrementAndClamp,
    Invert,
    IncrementAndWrap,
    DecrementAndWrap,

    Count
};

// =================================================================================================
// Sampler Create Info
// =================================================================================================

export struct RHISamplerCreateInfo {
    // Filtering
    RHISamplerFilter MagFilter = RHISamplerFilter::Linear;
    RHISamplerFilter MinFilter = RHISamplerFilter::Linear;
    RHISamplerMipmapMode MipmapMode = RHISamplerMipmapMode::Linear;

    // Address modes
    RHISamplerAddressMode AddressModeU = RHISamplerAddressMode::Repeat;
    RHISamplerAddressMode AddressModeV = RHISamplerAddressMode::Repeat;
    RHISamplerAddressMode AddressModeW = RHISamplerAddressMode::Repeat;

    // LOD settings
    float MipLodBias = 0.0f;
    float MinLod = 0.0f;
    float MaxLod = 1000.0f; // VK_LOD_CLAMP_NONE

    // Anisotropy
    bool AnisotropyEnable = false;
    float MaxAnisotropy = 1.0f;

    // Compare operations (for shadow mapping, etc.)
    bool CompareEnable = false;
    RHICompareOp CompareOp = RHICompareOp::Never;

    // Border color
    RHIBorderColor BorderColor = RHIBorderColor::FloatOpaqueBlack;

    // Unnormalized coordinates
    bool UnnormalizedCoordinates = false;
};

// =================================================================================================
// Sampler Class
// =================================================================================================

export class IGE_API RHISampler : public RHIResource {
public:
    ~RHISampler() override = default;

protected:
    RHISampler(const RHISamplerCreateInfo& info) : RHIResource(RHIResourceType::Sampler) {}
};

// =================================================================================================
// Default Sampler Types
// =================================================================================================

// export enum class RHIDefaultSamplerType : uint32 {
//     LinearClamp = 0,
//     LinearRepeat,
//     LinearMirror,
//     NearestClamp,
//     NearestRepeat,
//     NearestMirror,
//     Anisotropic2xClamp,
//     Anisotropic4xClamp,
//     Anisotropic8xClamp,
//     Anisotropic16xClamp,
//     Shadow, // For shadow mapping with comparison
//
//     Count
// };

} // namespace iGe
