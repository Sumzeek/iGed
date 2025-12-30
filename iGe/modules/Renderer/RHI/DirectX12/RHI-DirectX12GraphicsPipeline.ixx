module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12GraphicsPipeline;
import :RHIGraphicsPipeline;

namespace iGe
{

// =================================================================================================
// DirectX12GraphicsPipeline
// =================================================================================================

export class IGE_API DirectX12GraphicsPipeline : public RHIGraphicsPipeline {
public:
    DirectX12GraphicsPipeline(ID3D12Device* device, const RHIGraphicsPipelineCreateInfo& info);
    ~DirectX12GraphicsPipeline() override;

    // Getters
    ID3D12PipelineState* GetNativePSO() const { return m_PipelineState.Get(); }
    ID3D12RootSignature* GetRootSignature() const { return m_RootSignature.Get(); }
    D3D_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_PrimitiveTopology; }
    void* GetNativeHandle() const override { return m_PipelineState.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
    D3D_PRIMITIVE_TOPOLOGY m_PrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};

} // namespace iGe
#endif
