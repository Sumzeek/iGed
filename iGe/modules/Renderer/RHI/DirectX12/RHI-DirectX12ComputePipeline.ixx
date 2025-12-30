module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12ComputePipeline;
import :RHIComputePipeline;
import :DirectX12Shader;

namespace iGe
{

// =================================================================================================
// DirectX12ComputePipeline
// =================================================================================================

export class IGE_API DirectX12ComputePipeline : public RHIComputePipeline {
public:
    DirectX12ComputePipeline(ID3D12Device* device, const RHIComputePipelineCreateInfo& info);
    ~DirectX12ComputePipeline() override = default;

    ID3D12PipelineState* GetNativePSO() const { return m_PipelineState.Get(); }
    ID3D12RootSignature* GetRootSignature() const { return m_RootSignature.Get(); }
    void* GetNativeHandle() const override { return m_PipelineState.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
};

} // namespace iGe
#endif
