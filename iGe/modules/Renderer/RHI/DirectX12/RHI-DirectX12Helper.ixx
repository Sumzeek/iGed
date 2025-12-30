module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <dxgi1_4.h>

export module iGe.RHI:DirectX12Helper;
import :RHITexture;
import :RHIRenderPass;
import :RHISampler;
import :RHIDescriptor;
import :RHIResource;
import :RHIShader;
import :RHIQueue;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// Format Conversions
// =================================================================================================

export inline DXGI_FORMAT RHIFormatToDXGIFormat(RHIFormat format) {
    switch (format) {
        case RHIFormat::R8Srgb:
            return DXGI_FORMAT_R8_UNORM; // DXGI doesn't have R8_SRGB, usually handled in shader or view
        case RHIFormat::R8G8Srgb:
            return DXGI_FORMAT_R8G8_UNORM;
        case RHIFormat::R8G8B8A8Srgb:
            return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
        case RHIFormat::B8G8R8A8Srgb:
            return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;

        case RHIFormat::R16SFloat:
            return DXGI_FORMAT_R16_FLOAT;
        case RHIFormat::R16G16SFloat:
            return DXGI_FORMAT_R16G16_FLOAT;
        case RHIFormat::R16G16B16A16SFloat:
            return DXGI_FORMAT_R16G16B16A16_FLOAT;
        case RHIFormat::R32SFloat:
            return DXGI_FORMAT_R32_FLOAT;
        case RHIFormat::R32G32SFloat:
            return DXGI_FORMAT_R32G32_FLOAT;
        case RHIFormat::R32G32B32SFloat:
            return DXGI_FORMAT_R32G32B32_FLOAT;
        case RHIFormat::R32G32B32A32SFloat:
            return DXGI_FORMAT_R32G32B32A32_FLOAT;

        case RHIFormat::R8UNorm:
            return DXGI_FORMAT_R8_UNORM;
        case RHIFormat::R8G8UNorm:
            return DXGI_FORMAT_R8G8_UNORM;
        case RHIFormat::R8G8B8A8UNorm:
            return DXGI_FORMAT_R8G8B8A8_UNORM;
        case RHIFormat::R16UNorm:
            return DXGI_FORMAT_R16_UNORM;
        case RHIFormat::R16G16UNorm:
            return DXGI_FORMAT_R16G16_UNORM;
        case RHIFormat::R16G16B16A16UNorm:
            return DXGI_FORMAT_R16G16B16A16_UNORM;

        case RHIFormat::R8SNorm:
            return DXGI_FORMAT_R8_SNORM;
        case RHIFormat::R8G8SNorm:
            return DXGI_FORMAT_R8G8_SNORM;
        case RHIFormat::R8G8B8A8SNorm:
            return DXGI_FORMAT_R8G8B8A8_SNORM;
        case RHIFormat::R16SNorm:
            return DXGI_FORMAT_R16_SNORM;
        case RHIFormat::R16G16SNorm:
            return DXGI_FORMAT_R16G16_SNORM;
        case RHIFormat::R16G16B16A16SNorm:
            return DXGI_FORMAT_R16G16B16A16_SNORM;

        case RHIFormat::R8UInt:
            return DXGI_FORMAT_R8_UINT;
        case RHIFormat::R8G8UInt:
            return DXGI_FORMAT_R8G8_UINT;
        case RHIFormat::R8G8B8A8UInt:
            return DXGI_FORMAT_R8G8B8A8_UINT;
        case RHIFormat::R16UInt:
            return DXGI_FORMAT_R16_UINT;
        case RHIFormat::R16G16UInt:
            return DXGI_FORMAT_R16G16_UINT;
        case RHIFormat::R16G16B16A16UInt:
            return DXGI_FORMAT_R16G16B16A16_UINT;
        case RHIFormat::R32UInt:
            return DXGI_FORMAT_R32_UINT;
        case RHIFormat::R32G32UInt:
            return DXGI_FORMAT_R32G32_UINT;
        case RHIFormat::R32G32B32UInt:
            return DXGI_FORMAT_R32G32B32_UINT;
        case RHIFormat::R32G32B32A32UInt:
            return DXGI_FORMAT_R32G32B32A32_UINT;

        case RHIFormat::R8SInt:
            return DXGI_FORMAT_R8_SINT;
        case RHIFormat::R8G8SInt:
            return DXGI_FORMAT_R8G8_SINT;
        case RHIFormat::R8G8B8A8SInt:
            return DXGI_FORMAT_R8G8B8A8_SINT;
        case RHIFormat::R16SInt:
            return DXGI_FORMAT_R16_SINT;
        case RHIFormat::R16G16SInt:
            return DXGI_FORMAT_R16G16_SINT;
        case RHIFormat::R16G16B16A16SInt:
            return DXGI_FORMAT_R16G16B16A16_SINT;
        case RHIFormat::R32SInt:
            return DXGI_FORMAT_R32_SINT;
        case RHIFormat::R32G32SInt:
            return DXGI_FORMAT_R32G32_SINT;
        case RHIFormat::R32G32B32SInt:
            return DXGI_FORMAT_R32G32B32_SINT;
        case RHIFormat::R32G32B32A32SInt:
            return DXGI_FORMAT_R32G32B32A32_SINT;

        case RHIFormat::D32SFloat:
            return DXGI_FORMAT_D32_FLOAT;
        case RHIFormat::D32SFloatS8UInt:
            return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
        case RHIFormat::D24UNormS8UInt:
            return DXGI_FORMAT_D24_UNORM_S8_UINT;

        default:
            return DXGI_FORMAT_UNKNOWN;
    }
}

export inline bool IsDepthFormat(RHIFormat format) {
    return format == RHIFormat::D32SFloat || format == RHIFormat::D32SFloatS8UInt ||
           format == RHIFormat::D24UNormS8UInt;
}

export inline bool IsStencilFormat(RHIFormat format) {
    return format == RHIFormat::D32SFloatS8UInt || format == RHIFormat::D24UNormS8UInt;
}

export inline bool IsDepthStencilFormat(RHIFormat format) {
    return format == RHIFormat::D32SFloatS8UInt || format == RHIFormat::D24UNormS8UInt;
}

export inline D3D12_RESOURCE_STATES GetDX12State(RHILayout layout) {
    switch (layout) {
        case RHILayout::Undefined:
            return D3D12_RESOURCE_STATE_COMMON;
        case RHILayout::General:
            return D3D12_RESOURCE_STATE_COMMON;
        case RHILayout::ColorAttachment:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case RHILayout::DepthStencilAttachment:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case RHILayout::DepthStencilReadOnly:
            return D3D12_RESOURCE_STATE_DEPTH_READ;
        case RHILayout::ShaderReadOnly:
            return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        case RHILayout::TransferSrc:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case RHILayout::TransferDst:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case RHILayout::Present:
            return D3D12_RESOURCE_STATE_PRESENT;
        case RHILayout::Common:
            return D3D12_RESOURCE_STATE_COMMON;
        default:
            return D3D12_RESOURCE_STATE_COMMON;
    }
}

// =================================================================================================
// Sampler Conversions
// =================================================================================================

export inline D3D12_FILTER GetDX12Filter(RHISamplerFilter minFilter, RHISamplerFilter magFilter,
                                         RHISamplerMipmapMode mipMode, bool anisotropy, bool comparison) {
    // Build the filter based on min/mag/mip settings
    if (anisotropy) { return comparison ? D3D12_FILTER_COMPARISON_ANISOTROPIC : D3D12_FILTER_ANISOTROPIC; }

    uint32 filter = 0;

    // Mip filter
    if (mipMode == RHISamplerMipmapMode::Linear) {
        filter |= D3D12_FILTER_TYPE_LINEAR << D3D12_MIP_FILTER_SHIFT;
    } else {
        filter |= D3D12_FILTER_TYPE_POINT << D3D12_MIP_FILTER_SHIFT;
    }

    // Mag filter
    if (magFilter == RHISamplerFilter::Linear || magFilter == RHISamplerFilter::CubicImg) {
        filter |= D3D12_FILTER_TYPE_LINEAR << D3D12_MAG_FILTER_SHIFT;
    } else {
        filter |= D3D12_FILTER_TYPE_POINT << D3D12_MAG_FILTER_SHIFT;
    }

    // Min filter
    if (minFilter == RHISamplerFilter::Linear || minFilter == RHISamplerFilter::CubicImg) {
        filter |= D3D12_FILTER_TYPE_LINEAR << D3D12_MIN_FILTER_SHIFT;
    } else {
        filter |= D3D12_FILTER_TYPE_POINT << D3D12_MIN_FILTER_SHIFT;
    }

    if (comparison) { filter |= D3D12_FILTER_REDUCTION_TYPE_COMPARISON << D3D12_FILTER_REDUCTION_TYPE_SHIFT; }

    return static_cast<D3D12_FILTER>(filter);
}

export inline D3D12_TEXTURE_ADDRESS_MODE GetDX12AddressMode(RHISamplerAddressMode mode) {
    switch (mode) {
        case RHISamplerAddressMode::Repeat:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
        case RHISamplerAddressMode::MirroredRepeat:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
        case RHISamplerAddressMode::ClampToEdge:
            return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
        case RHISamplerAddressMode::ClampToBorder:
            return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        case RHISamplerAddressMode::MirrorClampToEdge:
            return D3D12_TEXTURE_ADDRESS_MODE_MIRROR_ONCE;
        default:
            return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    }
}

export inline D3D12_COMPARISON_FUNC GetDX12ComparisonFunc(RHICompareOp op) {
    switch (op) {
        case RHICompareOp::Never:
            return D3D12_COMPARISON_FUNC_NEVER;
        case RHICompareOp::Less:
            return D3D12_COMPARISON_FUNC_LESS;
        case RHICompareOp::Equal:
            return D3D12_COMPARISON_FUNC_EQUAL;
        case RHICompareOp::LessOrEqual:
            return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case RHICompareOp::Greater:
            return D3D12_COMPARISON_FUNC_GREATER;
        case RHICompareOp::NotEqual:
            return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case RHICompareOp::GreaterOrEqual:
            return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case RHICompareOp::Always:
            return D3D12_COMPARISON_FUNC_ALWAYS;
        default:
            return D3D12_COMPARISON_FUNC_NEVER;
    }
}

export inline void GetDX12BorderColor(RHIBorderColor color, float outColor[4]) {
    switch (color) {
        case RHIBorderColor::FloatTransparentBlack:
        case RHIBorderColor::IntTransparentBlack:
            outColor[0] = outColor[1] = outColor[2] = outColor[3] = 0.0f;
            break;
        case RHIBorderColor::FloatOpaqueBlack:
        case RHIBorderColor::IntOpaqueBlack:
            outColor[0] = outColor[1] = outColor[2] = 0.0f;
            outColor[3] = 1.0f;
            break;
        case RHIBorderColor::FloatOpaqueWhite:
        case RHIBorderColor::IntOpaqueWhite:
            outColor[0] = outColor[1] = outColor[2] = outColor[3] = 1.0f;
            break;
        default:
            outColor[0] = outColor[1] = outColor[2] = 0.0f;
            outColor[3] = 1.0f;
    }
}

export inline D3D12_STATIC_BORDER_COLOR GetDX12StaticBorderColor(RHIBorderColor color) {
    switch (color) {
        case RHIBorderColor::FloatTransparentBlack:
        case RHIBorderColor::IntTransparentBlack:
            return D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
        case RHIBorderColor::FloatOpaqueBlack:
        case RHIBorderColor::IntOpaqueBlack:
            return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
        case RHIBorderColor::FloatOpaqueWhite:
        case RHIBorderColor::IntOpaqueWhite:
            return D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        default:
            return D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
    }
}

// =================================================================================================
// Shader Visibility Conversions
// =================================================================================================

export inline D3D12_SHADER_VISIBILITY GetDX12ShaderVisibility(Flags<RHIShaderStage> stages) {
    // If multiple stages, use D3D12_SHADER_VISIBILITY_ALL
    uint32 count = 0;
    D3D12_SHADER_VISIBILITY visibility = D3D12_SHADER_VISIBILITY_ALL;

    if (stages.HasFlag(RHIShaderStage::Vertex)) {
        count++;
        visibility = D3D12_SHADER_VISIBILITY_VERTEX;
    }
    if (stages.HasFlag(RHIShaderStage::Fragment)) {
        count++;
        visibility = D3D12_SHADER_VISIBILITY_PIXEL;
    }
    if (stages.HasFlag(RHIShaderStage::Geometry)) {
        count++;
        visibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
    }
    if (stages.HasFlag(RHIShaderStage::TessControl)) {
        count++;
        visibility = D3D12_SHADER_VISIBILITY_HULL;
    }
    if (stages.HasFlag(RHIShaderStage::TessEvaluation)) {
        count++;
        visibility = D3D12_SHADER_VISIBILITY_DOMAIN;
    }
    // if (stages.HasFlag(RHIShaderStage::Mesh)) {
    //     count++;
    //     visibility = D3D12_SHADER_VISIBILITY_MESH;
    // }
    // if (stages.HasFlag(RHIShaderStage::Amplification)) {
    //     count++;
    //     visibility = D3D12_SHADER_VISIBILITY_AMPLIFICATION;
    // }

    if (count > 1 || stages.HasFlag(RHIShaderStage::Compute)) { return D3D12_SHADER_VISIBILITY_ALL; }

    return visibility;
}

// =================================================================================================
// Descriptor Range Type Conversions
// =================================================================================================

export inline D3D12_DESCRIPTOR_RANGE_TYPE GetDX12DescriptorRangeType(RHIDescriptorType type) {
    switch (type) {
        case RHIDescriptorType::Sampler:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
        case RHIDescriptorType::CombinedImageSampler:
        case RHIDescriptorType::SampledImage:
        case RHIDescriptorType::UniformTexelBuffer:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
        case RHIDescriptorType::StorageImage:
        case RHIDescriptorType::StorageTexelBuffer:
        case RHIDescriptorType::StorageBuffer:
        case RHIDescriptorType::StorageBufferDynamic:
            return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        case RHIDescriptorType::UniformBuffer:
        case RHIDescriptorType::UniformBufferDynamic:
            return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        default:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }
}

// =================================================================================================
// Command List Type Conversions
// =================================================================================================

export inline D3D12_COMMAND_LIST_TYPE GetD3D12CommandListType(RHIQueueType type) {
    switch (type) {
        case RHIQueueType::Graphics:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
        case RHIQueueType::Compute:
            return D3D12_COMMAND_LIST_TYPE_COMPUTE;
        case RHIQueueType::Transfer:
            return D3D12_COMMAND_LIST_TYPE_COPY;
        default:
            return D3D12_COMMAND_LIST_TYPE_DIRECT;
    }
}

} // namespace iGe
#endif
