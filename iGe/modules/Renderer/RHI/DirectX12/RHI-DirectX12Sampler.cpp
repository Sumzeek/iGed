module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12Sampler;
import :DirectX12Helper;

namespace iGe
{

DirectX12Sampler::DirectX12Sampler(ID3D12Device* device, const RHISamplerCreateInfo& info) : RHISampler(info) {

    // Build the D3D12 sampler desc
    m_SamplerDesc.Filter =
            GetDX12Filter(info.MinFilter, info.MagFilter, info.MipmapMode, info.AnisotropyEnable, info.CompareEnable);
    m_SamplerDesc.AddressU = GetDX12AddressMode(info.AddressModeU);
    m_SamplerDesc.AddressV = GetDX12AddressMode(info.AddressModeV);
    m_SamplerDesc.AddressW = GetDX12AddressMode(info.AddressModeW);
    m_SamplerDesc.MipLODBias = info.MipLodBias;
    m_SamplerDesc.MaxAnisotropy = static_cast<UINT>(info.MaxAnisotropy);
    m_SamplerDesc.ComparisonFunc =
            info.CompareEnable ? GetDX12ComparisonFunc(info.CompareOp) : D3D12_COMPARISON_FUNC_NONE;
    m_SamplerDesc.MinLOD = info.MinLod;
    m_SamplerDesc.MaxLOD = info.MaxLod;

    // Border color
    GetDX12BorderColor(info.BorderColor, m_SamplerDesc.BorderColor);
}

DirectX12Sampler::~DirectX12Sampler() {
    // The descriptor handle should be freed by the descriptor heap manager
}

} // namespace iGe
#endif
