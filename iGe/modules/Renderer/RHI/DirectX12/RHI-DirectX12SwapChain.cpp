module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <dxgi1_6.h>
    #include <stdexcept>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12SwapChain;
import :DirectX12Helper;
import :DirectX12Fence;
import :DirectX12Semaphore;

namespace iGe
{

using Microsoft::WRL::ComPtr;

// =================================================================================================
// DirectX12SwapChain
// =================================================================================================

DirectX12SwapChain::DirectX12SwapChain(IDXGIFactory4* factory, ID3D12Device* device, ID3D12CommandQueue* presentQueue,
                                       const RHISwapChainCreateInfo& info)
    : RHISwapChain(info), m_Device(device) {
    if (!factory || !device || !presentQueue) {
        Internal::LogError("DirectX12SwapChain: Invalid factory, device or present queue");
    }

    // Default to VSync enabled
    m_VSync = true;

    // Check for tearing support (for variable refresh rate displays)
    BOOL allowTearing = FALSE;
    ComPtr<IDXGIFactory5> factory5;
    if (SUCCEEDED(factory->QueryInterface(IID_PPV_ARGS(&factory5)))) {
        if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing,
                                                 sizeof(allowTearing)))) {
            allowTearing = FALSE;
        }
    }
    m_TearingSupported = (allowTearing == TRUE);

    // Create swap chain
    {
        // Get the HWND from the surface
        m_Hwnd = static_cast<HWND>(info.Surface->GetNativeWindowHandle());
        if (!m_Hwnd) { Internal::LogError("DirectX12SwapChain: Invalid window handle for swap chain creation"); }

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = m_Extent.Width;
        swapChainDesc.Height = m_Extent.Height;
        swapChainDesc.Format = GetDXGIFormat();
        swapChainDesc.Stereo = FALSE;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = m_ImageCount;
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags = m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

        ComPtr<IDXGISwapChain1> swapChain1;
        HRESULT hr = factory->CreateSwapChainForHwnd(presentQueue, m_Hwnd, &swapChainDesc,
                                                     nullptr, // Windowed mode
                                                     nullptr, // No restrict to output
                                                     &swapChain1);

        if (FAILED(hr)) { Internal::LogError("DirectX12SwapChain: Failed to create swap chain"); }

        // Disable Alt+Enter fullscreen toggle
        factory->MakeWindowAssociation(m_Hwnd, DXGI_MWA_NO_ALT_ENTER);

        // Get IDXGISwapChain4 for additional features
        if (FAILED(swapChain1.As(&m_SwapChain))) {
            Internal::LogError("DirectX12SwapChain: Failed to get IDXGISwapChain4 interface");
        }
    }

    CreateBackBufferResources();
}

DirectX12SwapChain::~DirectX12SwapChain() { ReleaseBackBufferResources(); }

DXGI_FORMAT DirectX12SwapChain::GetDXGIFormat() const { return RHIFormatToDXGIFormat(m_Format); }

uint32 DirectX12SwapChain::AcquireNextImage(RHISemaphore* signalSemaphore, RHIFence* signalFence) {
    // In D3D12, the swap chain automatically manages the current back buffer index
    uint32 currentFrameIndex = m_SwapChain->GetCurrentBackBufferIndex();

    // D3D12 doesn't have the same acquire semantics as Vulkan
    // The image is immediately available after GetCurrentBackBufferIndex()
    // Synchronization is handled through command queue fences

    // Signal the fence if provided (D3D12 specific)
    if (signalFence) {
        auto* dx12Fence = static_cast<DirectX12Fence*>(signalFence);
        // In D3D12, we would typically signal the fence after the present queue completes
        // For acquire semantics, we can signal immediately since image is already available
    }

    return currentFrameIndex;
}

RHITexture* DirectX12SwapChain::GetBackBufferTexture(uint32 index) const {
    if (index >= m_BackBufferTextures.size()) { return nullptr; }
    return m_BackBufferTextures[index].get();
}

RHITextureView* DirectX12SwapChain::GetBackBufferView(uint32 index) const {
    if (index >= m_BackBufferViews.size()) { return nullptr; }
    return m_BackBufferViews[index].get();
}

void DirectX12SwapChain::Present(std::span<RHISemaphore* const> waitSemaphores) {
    UINT syncInterval = m_VSync ? 1 : 0;
    UINT presentFlags = 0;

    // Use tearing if supported and not using VSync
    if (!m_VSync && m_TearingSupported) { presentFlags |= DXGI_PRESENT_ALLOW_TEARING; }

    // Note: D3D12 doesn't have native semaphore support for present
    // Synchronization is typically handled through command queue fences
    // The waitSemaphores parameter is provided for API compatibility with Vulkan

    HRESULT hr = m_SwapChain->Present(syncInterval, presentFlags);

    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET) {
        // Handle device lost scenario - this should trigger device recreation
        Internal::LogError("DirectX12SwapChain: Device removed or reset during present");
    } else if (FAILED(hr)) {
        Internal::LogError("DirectX12SwapChain: Failed to present swap chain");
    }
}

void DirectX12SwapChain::Resize(uint32 width, uint32 height) {
    if (width == 0 || height == 0) {
        return; // Minimized window, skip resize
    }

    // Update extent in base info
    m_Extent.Width = width;
    m_Extent.Height = height;

    // Release existing back buffer resources
    ReleaseBackBufferResources();

    // Resize swap chain buffers
    UINT flags = m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
    HRESULT hr = m_SwapChain->ResizeBuffers(m_ImageCount, width, height, GetDXGIFormat(), flags);

    if (FAILED(hr)) { Internal::LogError("DirectX12SwapChain: Failed to resize swap chain buffers"); }

    // Recreate back buffer resources
    CreateBackBufferResources();
}

void DirectX12SwapChain::CreateBackBufferResources() {
    uint32 imageCount = m_ImageCount;
    m_BackBufferTextures.clear();
    m_BackBufferViews.clear();
    m_BackBufferTextures.reserve(imageCount);
    m_BackBufferViews.reserve(imageCount);

    for (uint32 i = 0; i < imageCount; ++i) {
        // Get back buffer resource from swap chain
        ComPtr<ID3D12Resource> backBuffer;
        HRESULT hr = m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer));
        if (FAILED(hr)) { Internal::LogError("DirectX12SwapChain: Failed to get swap chain buffer"); }

        // Create texture info for the back buffer
        RHITextureCreateInfo textureInfo = {};
        textureInfo.Extent.Width = m_Extent.Width;
        textureInfo.Extent.Height = m_Extent.Height;
        textureInfo.Extent.Depth = 1;
        textureInfo.MipLevels = 1;
        textureInfo.ArrayLayers = 1;
        textureInfo.Format = m_Format;
        textureInfo.MemoryUsage = RHIMemoryUsage::GpuOnly;
        textureInfo.Usage = RHITextureUsageFlagBits::ColorAttachment; // Back buffer is used as render target

        // Create wrapper texture for the back buffer
        m_BackBufferTextures.push_back(CreateScope<DirectX12Texture>(m_Device, textureInfo, backBuffer.Get()));

        // Create texture view for rendering
        RHITextureViewCreateInfo viewInfo = {};
        viewInfo.ViewType = RHITextureViewType::View2D;
        viewInfo.Format = m_Format;

        m_BackBufferViews.push_back(
                CreateScope<DirectX12TextureView>(m_Device, viewInfo, m_BackBufferTextures[i].get()));
    }
}

void DirectX12SwapChain::ReleaseBackBufferResources() {
    m_BackBufferViews.clear();
    m_BackBufferTextures.clear();
}

} // namespace iGe
#endif
