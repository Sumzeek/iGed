module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12Texture;
import :RHITexture;
import iGe.Common;

namespace iGe
{

export class IGE_API DirectX12Texture : public RHITexture {
public:
    // Constructor for creating a new texture
    DirectX12Texture(ID3D12Device* device, const RHITextureCreateInfo& info);

    // Constructor for wrapping an existing resource (e.g. SwapChain backbuffer)
    DirectX12Texture(ID3D12Device* device, const RHITextureCreateInfo& info, ID3D12Resource* resource);

    ~DirectX12Texture() override = default;

    void* GetNativeHandle() const override { return m_Resource.Get(); }

    ID3D12Resource* GetResource() const { return m_Resource.Get(); }

    // Helpers for View creation (to be called by Framebuffer/RenderPass logic)
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const { return m_RTVHandle; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSV() const { return m_DSVHandle; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return m_SRVHandle; }

    ID3D12DescriptorHeap* GetSRVHeap() const { return m_SRVHeap.Get(); }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVHandleGPU() const { return m_SRVHeap->GetGPUDescriptorHandleForHeapStart(); }

    D3D12_RESOURCE_STATES GetState() const { return m_State; }
    void SetState(D3D12_RESOURCE_STATES state) { m_State = state; }

private:
    void CreateViews(Microsoft::WRL::ComPtr<ID3D12Device> device);

    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
    D3D12_RESOURCE_STATES m_State = D3D12_RESOURCE_STATE_COMMON;

    // Simple descriptor management: each texture owns its own heaps for views (Inefficient but simple)
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SRVHeap;
    D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle = {0};
    D3D12_CPU_DESCRIPTOR_HANDLE m_DSVHandle = {0};
    D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle = {0};
};

} // namespace iGe
#endif
