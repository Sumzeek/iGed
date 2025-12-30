module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12Texture;
import :DirectX12Helper;

namespace iGe
{

// =================================================================================================
// DirectX12Texture
// =================================================================================================

DirectX12Texture::DirectX12Texture(ID3D12Device* device, const RHITextureCreateInfo& info) : RHITexture(info) {
    D3D12_RESOURCE_DESC desc = {};
    desc.Alignment = 0;
    desc.Width = info.Extent.Width;
    desc.Height = info.Extent.Height;
    desc.MipLevels = static_cast<UINT16>(info.MipLevels);
    desc.Format = RHIFormatToDXGIFormat(info.Format);
    desc.SampleDesc.Count = 1; // TODO: Support MSAA
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

    if (info.Extent.Depth > 1) {
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;
        desc.DepthOrArraySize = static_cast<UINT16>(info.Extent.Depth);
    } else {
        desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        desc.DepthOrArraySize = static_cast<UINT16>(info.ArrayLayers);
    }

    // Infer flags
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;
    if (IsDepthFormat(info.Format)) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    } else {
        // Assume color attachment if not depth
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;
    heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProps.CreationNodeMask = 1;
    heapProps.VisibleNodeMask = 1;

    D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

    D3D12_CLEAR_VALUE clearValue = {};
    D3D12_CLEAR_VALUE* pClearValue = nullptr;

    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) {
        clearValue.Format = desc.Format;
        clearValue.Color[0] = 0.0f;
        clearValue.Color[1] = 0.0f;
        clearValue.Color[2] = 0.0f;
        clearValue.Color[3] = 0.0f;
        pClearValue = &clearValue;
    } else if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) {
        clearValue.Format = desc.Format;
        clearValue.DepthStencil.Depth = 1.0f;
        clearValue.DepthStencil.Stencil = 0;
        pClearValue = &clearValue;
    }

    HRESULT hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &desc, initialState, pClearValue,
                                                 IID_PPV_ARGS(&m_Resource));

    if (FAILED(hr)) { Internal::LogError("Failed to create texture resource"); }

    CreateViews(device);
}

DirectX12Texture::DirectX12Texture(ID3D12Device* device, const RHITextureCreateInfo& info, ID3D12Resource* resource)
    : RHITexture(info), m_Resource(resource) {
    CreateViews(device);
}

void DirectX12Texture::CreateViews(Microsoft::WRL::ComPtr<ID3D12Device> device) {
    D3D12_RESOURCE_DESC desc = m_Resource->GetDesc();

    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = 1;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        if (FAILED(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap)))) {
            Internal::LogError("Failed to create RTV heap");
        }

        m_RTVHandle = m_RTVHeap->GetCPUDescriptorHandleForHeapStart();
        device->CreateRenderTargetView(m_Resource.Get(), nullptr, m_RTVHandle);
    }

    if (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) {
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        if (FAILED(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&m_DSVHeap)))) {
            Internal::LogError("Failed to create DSV heap");
        }

        m_DSVHandle = m_DSVHeap->GetCPUDescriptorHandleForHeapStart();
        device->CreateDepthStencilView(m_Resource.Get(), nullptr, m_DSVHandle);
    }

    if (!(desc.Flags & D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE)) {
        // Create SRV
        D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
        srvHeapDesc.NumDescriptors = 1;
        srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        if (FAILED(device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SRVHeap)))) {
            Internal::LogError("Failed to create SRV heap");
        }

        m_SRVHandle = m_SRVHeap->GetCPUDescriptorHandleForHeapStart();

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = desc.Format;

        // Basic depth format adjustment (This usually requires Typeless resource creation to work fully correctly in DX12,
        // but for now let's hope the driver is lenient or the user isn't sampling depth yet)
        if (srvDesc.Format == DXGI_FORMAT_D32_FLOAT) srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
        else if (srvDesc.Format == DXGI_FORMAT_D16_UNORM)
            srvDesc.Format = DXGI_FORMAT_R16_UNORM;

        if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
            if (desc.DepthOrArraySize > 1) {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
                srvDesc.Texture2DArray.MipLevels = desc.MipLevels;
                srvDesc.Texture2DArray.ArraySize = desc.DepthOrArraySize;
            } else {
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = desc.MipLevels;
            }
        } else if (desc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
            srvDesc.Texture3D.MipLevels = desc.MipLevels;
        }

        device->CreateShaderResourceView(m_Resource.Get(), &srvDesc, m_SRVHandle);
    }
}

} // namespace iGe
#endif
