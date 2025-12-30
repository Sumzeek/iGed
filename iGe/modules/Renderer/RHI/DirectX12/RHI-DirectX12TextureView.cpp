module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12TextureView;

namespace iGe
{

using Microsoft::WRL::ComPtr;

// Helper function to map RHIComponentSwizzle to D3D12 component
inline D3D12_SHADER_COMPONENT_MAPPING MapSwizzle(RHIComponentSwizzle swizzle,
                                                 D3D12_SHADER_COMPONENT_MAPPING defaultChannel) {
    switch (swizzle) {
        case RHIComponentSwizzle::Identity:
            return defaultChannel;
        case RHIComponentSwizzle::Zero:
            return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_0;
        case RHIComponentSwizzle::One:
            return D3D12_SHADER_COMPONENT_MAPPING_FORCE_VALUE_1;
        case RHIComponentSwizzle::R:
            return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0;
        case RHIComponentSwizzle::G:
            return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1;
        case RHIComponentSwizzle::B:
            return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2;
        case RHIComponentSwizzle::A:
            return D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3;
        default:
            return defaultChannel;
    }
}

// Helper function to convert RHIComponentMapping to D3D12_SHADER_4_COMPONENT_MAPPING
inline UINT MapComponentMapping(const RHIComponentMapping& mapping) {
    return D3D12_ENCODE_SHADER_4_COMPONENT_MAPPING(
            MapSwizzle(mapping.R, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_0),
            MapSwizzle(mapping.G, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_1),
            MapSwizzle(mapping.B, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_2),
            MapSwizzle(mapping.A, D3D12_SHADER_COMPONENT_MAPPING_FROM_MEMORY_COMPONENT_3));
}

DirectX12TextureView::DirectX12TextureView(ID3D12Device* device, const RHITextureViewCreateInfo& info,
                                           DirectX12Texture* texture)
    : RHITextureView(info) {
    if (!device || !texture) { Internal::LogError("DirectX12TextureView: Invalid device or texture"); }

    // Determine format from info or fallback to texture format
    RHIFormat viewFormat = info.Format != RHIFormat::Unknown ? info.Format : texture->GetFormat();

    // Create descriptor heap for SRV (non-shader-visible for CPU staging, will be copied to shader-visible heap)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // Non-shader-visible for copy source
        heapDesc.NodeMask = 0;

        HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_SRVHeap));
        if (FAILED(hr)) { Internal::LogError("DirectX12TextureView: Failed to create SRV heap"); }

        m_SRVHandle = m_SRVHeap->GetCPUDescriptorHandleForHeapStart();

        // Build SRV description
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        DXGI_FORMAT dxFormat = RHIFormatToDXGIFormat(viewFormat);

        // Convert depth formats to their SRV-compatible equivalents
        switch (dxFormat) {
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_R32_TYPELESS:
                srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
                break;
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24G8_TYPELESS:
                srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                break;
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32G8X24_TYPELESS:
                srvDesc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                break;
            case DXGI_FORMAT_D16_UNORM:
            case DXGI_FORMAT_R16_TYPELESS:
                srvDesc.Format = DXGI_FORMAT_R16_UNORM;
                break;
            default:
                srvDesc.Format = dxFormat;
                break;
        }

        // Apply component swizzle mapping
        srvDesc.Shader4ComponentMapping = MapComponentMapping(info.Components);

        // Use texture properties for subresource specification
        uint32 mipLevels = texture->GetMipLevels();
        uint32 arrayLayers = texture->GetArrayLayers();

        switch (info.ViewType) {
            case RHITextureViewType::View1D:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                srvDesc.Texture1D.MostDetailedMip = 0;
                srvDesc.Texture1D.MipLevels = mipLevels;
                srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
                break;

            case RHITextureViewType::View1DArray:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                srvDesc.Texture1DArray.MostDetailedMip = 0;
                srvDesc.Texture1DArray.MipLevels = mipLevels;
                srvDesc.Texture1DArray.FirstArraySlice = 0;
                srvDesc.Texture1DArray.ArraySize = arrayLayers;
                break;

            case RHITextureViewType::View2D:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Texture2D.MipLevels = mipLevels;
                srvDesc.Texture2D.PlaneSlice = 0;
                srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
                break;

            case RHITextureViewType::View2DArray:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.MostDetailedMip = 0;
                srvDesc.Texture2DArray.MipLevels = mipLevels;
                srvDesc.Texture2DArray.FirstArraySlice = 0;
                srvDesc.Texture2DArray.ArraySize = arrayLayers;
                srvDesc.Texture2DArray.PlaneSlice = 0;
                break;

            case RHITextureViewType::ViewCube:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                srvDesc.TextureCube.MostDetailedMip = 0;
                srvDesc.TextureCube.MipLevels = mipLevels;
                srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
                break;

            case RHITextureViewType::ViewCubeArray:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                srvDesc.TextureCubeArray.MostDetailedMip = 0;
                srvDesc.TextureCubeArray.MipLevels = mipLevels;
                srvDesc.TextureCubeArray.First2DArrayFace = 0;
                srvDesc.TextureCubeArray.NumCubes = arrayLayers / 6;
                break;

            case RHITextureViewType::View3D:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                srvDesc.Texture3D.MostDetailedMip = 0;
                srvDesc.Texture3D.MipLevels = mipLevels;
                srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
                break;

            default:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Texture2D.MipLevels = mipLevels;
                break;
        }

        device->CreateShaderResourceView(texture->GetResource(), &srvDesc, m_SRVHandle);
    }

    // Create RTV/DSV based on texture usage and format
    auto textureUsage = texture->GetUsage();

    if (textureUsage.HasFlag(RHITextureUsageFlagBits::ColorAttachment)) {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.NodeMask = 0;

        HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_RTVHeap));
        if (FAILED(hr)) { Internal::LogError("DirectX12TextureView: Failed to create RTV heap"); }

        m_RTVHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();

        D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
        rtvDesc.Format = RHIFormatToDXGIFormat(viewFormat);

        uint32 arrayLayers = texture->GetArrayLayers();

        switch (info.ViewType) {
            case RHITextureViewType::View1D:
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
                rtvDesc.Texture1D.MipSlice = 0;
                break;

            case RHITextureViewType::View1DArray:
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
                rtvDesc.Texture1DArray.MipSlice = 0;
                rtvDesc.Texture1DArray.FirstArraySlice = 0;
                rtvDesc.Texture1DArray.ArraySize = arrayLayers;
                break;

            case RHITextureViewType::View2D:
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = 0;
                rtvDesc.Texture2D.PlaneSlice = 0;
                break;

            case RHITextureViewType::View2DArray:
            case RHITextureViewType::ViewCube:
            case RHITextureViewType::ViewCubeArray:
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
                rtvDesc.Texture2DArray.MipSlice = 0;
                rtvDesc.Texture2DArray.FirstArraySlice = 0;
                rtvDesc.Texture2DArray.ArraySize = arrayLayers;
                rtvDesc.Texture2DArray.PlaneSlice = 0;
                break;

            case RHITextureViewType::View3D:
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
                rtvDesc.Texture3D.MipSlice = 0;
                rtvDesc.Texture3D.FirstWSlice = 0;
                rtvDesc.Texture3D.WSize = texture->GetDepth();
                break;

            default:
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = 0;
                break;
        }

        device->CreateRenderTargetView(texture->GetResource(), &rtvDesc, m_RTVHandle);
    }

    if (textureUsage.HasFlag(RHITextureUsageFlagBits::DepthStencilAttachment) || IsDepthFormat(viewFormat)) {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        heapDesc.NodeMask = 0;

        HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_DSVHeap));
        if (FAILED(hr)) { Internal::LogError("DirectX12TextureView: Failed to create DSV heap"); }

        m_DSVHandle = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();

        D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};

        // Get appropriate DSV format (depth formats need special handling)
        DXGI_FORMAT dxFormat = RHIFormatToDXGIFormat(viewFormat);
        switch (dxFormat) {
            case DXGI_FORMAT_R32_TYPELESS:
                dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
                break;
            case DXGI_FORMAT_R24G8_TYPELESS:
                dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                break;
            case DXGI_FORMAT_R32G8X24_TYPELESS:
                dsvDesc.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
                break;
            default:
                dsvDesc.Format = dxFormat;
                break;
        }

        uint32 arrayLayers = texture->GetArrayLayers();
        dsvDesc.Flags = D3D12_DSV_FLAG_NONE;

        switch (info.ViewType) {
            case RHITextureViewType::View1D:
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
                dsvDesc.Texture1D.MipSlice = 0;
                break;

            case RHITextureViewType::View1DArray:
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
                dsvDesc.Texture1DArray.MipSlice = 0;
                dsvDesc.Texture1DArray.FirstArraySlice = 0;
                dsvDesc.Texture1DArray.ArraySize = arrayLayers;
                break;

            case RHITextureViewType::View2D:
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsvDesc.Texture2D.MipSlice = 0;
                break;

            case RHITextureViewType::View2DArray:
            case RHITextureViewType::ViewCube:
            case RHITextureViewType::ViewCubeArray:
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
                dsvDesc.Texture2DArray.MipSlice = 0;
                dsvDesc.Texture2DArray.FirstArraySlice = 0;
                dsvDesc.Texture2DArray.ArraySize = arrayLayers;
                break;

            default:
                dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
                dsvDesc.Texture2D.MipSlice = 0;
                break;
        }

        device->CreateDepthStencilView(texture->GetResource(), &dsvDesc, m_DSVHandle);
    }

    // Create descriptor heap for SRV (non-shader-visible for CPU staging, will be copied to shader-visible heap)
    if (textureUsage.HasFlag(RHITextureUsageFlagBits::Storage)) {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        heapDesc.NumDescriptors = 1;
        heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // Non-shader-visible for copy source
        heapDesc.NodeMask = 0;

        HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_SRVHeap));
        if (FAILED(hr)) { Internal::LogError("DirectX12TextureView: Failed to create SRV heap"); }

        m_SRVHandle = m_SRVHeap->GetCPUDescriptorHandleForHeapStart();

        // Build SRV description
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        DXGI_FORMAT dxFormat = RHIFormatToDXGIFormat(viewFormat);

        // Convert depth formats to their SRV-compatible equivalents
        switch (dxFormat) {
            case DXGI_FORMAT_D32_FLOAT:
            case DXGI_FORMAT_R32_TYPELESS:
                srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
                break;
            case DXGI_FORMAT_D24_UNORM_S8_UINT:
            case DXGI_FORMAT_R24G8_TYPELESS:
                srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                break;
            case DXGI_FORMAT_D32_FLOAT_S8X24_UINT:
            case DXGI_FORMAT_R32G8X24_TYPELESS:
                srvDesc.Format = DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
                break;
            case DXGI_FORMAT_D16_UNORM:
            case DXGI_FORMAT_R16_TYPELESS:
                srvDesc.Format = DXGI_FORMAT_R16_UNORM;
                break;
            default:
                srvDesc.Format = dxFormat;
                break;
        }

        // Apply component swizzle mapping
        srvDesc.Shader4ComponentMapping = MapComponentMapping(info.Components);

        // Use texture properties for subresource specification
        uint32 mipLevels = texture->GetMipLevels();
        uint32 arrayLayers = texture->GetArrayLayers();

        switch (info.ViewType) {
            case RHITextureViewType::View1D:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
                srvDesc.Texture1D.MostDetailedMip = 0;
                srvDesc.Texture1D.MipLevels = mipLevels;
                srvDesc.Texture1D.ResourceMinLODClamp = 0.0f;
                break;

            case RHITextureViewType::View1DArray:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
                srvDesc.Texture1DArray.MostDetailedMip = 0;
                srvDesc.Texture1DArray.MipLevels = mipLevels;
                srvDesc.Texture1DArray.FirstArraySlice = 0;
                srvDesc.Texture1DArray.ArraySize = arrayLayers;
                break;

            case RHITextureViewType::View2D:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Texture2D.MipLevels = mipLevels;
                srvDesc.Texture2D.PlaneSlice = 0;
                srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
                break;

            case RHITextureViewType::View2DArray:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.MostDetailedMip = 0;
                srvDesc.Texture2DArray.MipLevels = mipLevels;
                srvDesc.Texture2DArray.FirstArraySlice = 0;
                srvDesc.Texture2DArray.ArraySize = arrayLayers;
                srvDesc.Texture2DArray.PlaneSlice = 0;
                break;

            case RHITextureViewType::ViewCube:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
                srvDesc.TextureCube.MostDetailedMip = 0;
                srvDesc.TextureCube.MipLevels = mipLevels;
                srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
                break;

            case RHITextureViewType::ViewCubeArray:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
                srvDesc.TextureCubeArray.MostDetailedMip = 0;
                srvDesc.TextureCubeArray.MipLevels = mipLevels;
                srvDesc.TextureCubeArray.First2DArrayFace = 0;
                srvDesc.TextureCubeArray.NumCubes = arrayLayers / 6;
                break;

            case RHITextureViewType::View3D:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
                srvDesc.Texture3D.MostDetailedMip = 0;
                srvDesc.Texture3D.MipLevels = mipLevels;
                srvDesc.Texture3D.ResourceMinLODClamp = 0.0f;
                break;

            default:
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MostDetailedMip = 0;
                srvDesc.Texture2D.MipLevels = mipLevels;
                break;
        }

        device->CreateShaderResourceView(texture->GetResource(), &srvDesc, m_SRVHandle);
    }
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectX12TextureView::GetSRVGpu() const {
    if (m_SRVHeap) { return m_SRVHeap->GetGPUDescriptorHandleForHeapStart(); }
    return D3D12_GPU_DESCRIPTOR_HANDLE{0};
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectX12TextureView::GetUAVGpu() const {
    if (m_UAVHeap) { return m_UAVHeap->GetGPUDescriptorHandleForHeapStart(); }
    return D3D12_GPU_DESCRIPTOR_HANDLE{0};
}

} // namespace iGe
#endif
