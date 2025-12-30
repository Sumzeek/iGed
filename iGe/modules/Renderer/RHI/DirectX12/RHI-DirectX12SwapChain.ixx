module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <dxgi1_6.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12SwapChain;
import :RHISwapChain;
import :DirectX12Texture;
import :DirectX12TextureView;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// DirectX12SwapChain
// =================================================================================================

export class IGE_API DirectX12SwapChain : public RHISwapChain {
public:
    DirectX12SwapChain(IDXGIFactory4* factory, ID3D12Device* device, ID3D12CommandQueue* presentQueue,
                       const RHISwapChainCreateInfo& info);
    ~DirectX12SwapChain() override;

    // Non-copyable, movable
    DirectX12SwapChain(const DirectX12SwapChain&) = delete;
    DirectX12SwapChain& operator=(const DirectX12SwapChain&) = delete;
    DirectX12SwapChain(DirectX12SwapChain&&) = default;
    DirectX12SwapChain& operator=(DirectX12SwapChain&&) = default;

    // RHISwapChain interface
    uint32 AcquireNextImage(RHISemaphore* signalSemaphore = nullptr, RHIFence* signalFence = nullptr) override;
    void Present(std::span<RHISemaphore* const> waitSemaphores = {}) override;
    void Resize(uint32 width, uint32 height) override;

    RHITexture* GetBackBufferTexture(uint32 index) const override;
    RHITextureView* GetBackBufferView(uint32 index) const override;

    // Native access
    void* GetNativeHandle() const override { return m_SwapChain.Get(); }
    IDXGISwapChain4* GetNativeSwapChain() const { return m_SwapChain.Get(); }

private:
    void CreateBackBufferResources();
    void ReleaseBackBufferResources();
    DXGI_FORMAT GetDXGIFormat() const;

    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_SwapChain;

    // Use for Resize
    ID3D12Device* m_Device;

    // Back buffer textures and views
    std::vector<Scope<DirectX12Texture>> m_BackBufferTextures;
    std::vector<Scope<DirectX12TextureView>> m_BackBufferViews;

    bool m_VSync = true;
    bool m_TearingSupported = false;
    HWND m_Hwnd = nullptr;
};

} // namespace iGe
#endif
