module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12ComputePipeline;
import :DirectX12Descriptor;

namespace iGe
{


// =================================================================================================
// DirectX12ComputePipeline
// =================================================================================================

DirectX12ComputePipeline::DirectX12ComputePipeline(ID3D12Device* device, const RHIComputePipelineCreateInfo& info)
    : RHIComputePipeline(info) {
    // Get root signature from pipeline layout
    if (info.pLayout) {
        auto* dxLayout = static_cast<const DirectX12PipelineLayout*>(info.pLayout);
        m_RootSignature = dxLayout->GetRootSignature();
    }

    if (!m_RootSignature) {
        Internal::LogError("Pipeline layout with root signature is required for compute pipeline");
        return;
    }

    D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_RootSignature.Get();

    // Set compute shader
    if (info.pComputeShader) {
        auto* dxShader = static_cast<const DirectX12Shader*>(info.pComputeShader);
        psoDesc.CS.pShaderBytecode = dxShader->GetBytecode();
        psoDesc.CS.BytecodeLength = dxShader->GetBytecodeSize();
    }

    HRESULT hr = device->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState));
    if (FAILED(hr)) { Internal::LogError("Failed to create compute pipeline state: {}", hr); }
}

} // namespace iGe
#endif
