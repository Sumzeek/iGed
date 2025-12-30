module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12TextureView;
import :DirectX12Texture;
import :DirectX12Helper;
import iGe.Common;

namespace iGe
{

export class IGE_API DirectX12TextureView : public RHITextureView {
public:
    DirectX12TextureView(ID3D12Device* device, const RHITextureViewCreateInfo& info, DirectX12Texture* texture);

    ~DirectX12TextureView() override = default;

    // Get various descriptor handles
    D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCpu() const { return m_SRVHandle; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGpu() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetUAVCpu() const { return m_UAVHandle; }
    D3D12_GPU_DESCRIPTOR_HANDLE GetUAVGpu() const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVCpu() const { return m_RTVHandle; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVCpu() const { return m_DSVHandle; }

private:
    // Descriptor heaps for this view
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SRVHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_UAVHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_DSVHeap;

    D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle = {0};
    D3D12_CPU_DESCRIPTOR_HANDLE m_UAVHandle = {0};
    D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle = {0};
    D3D12_CPU_DESCRIPTOR_HANDLE m_DSVHandle = {0};
};

} // namespace iGe
#endif
