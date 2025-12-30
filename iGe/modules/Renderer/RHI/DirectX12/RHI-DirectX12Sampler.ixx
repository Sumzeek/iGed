module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12Sampler;
import :RHISampler;
import iGe.Common;

namespace iGe
{

export class IGE_API DirectX12Sampler : public RHISampler {
public:
    DirectX12Sampler(ID3D12Device* device, const RHISamplerCreateInfo& info);
    ~DirectX12Sampler() override;

    // Get the sampler descriptor
    const D3D12_SAMPLER_DESC& GetDesc() const { return m_SamplerDesc; }

    // Get CPU descriptor handle (for staging heap)
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return m_CPUHandle; }

    // Set CPU handle (allocated from descriptor heap)
    void SetCPUHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) { m_CPUHandle = handle; }

private:
    D3D12_SAMPLER_DESC m_SamplerDesc = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHandle = {};
};

} // namespace iGe
#endif
