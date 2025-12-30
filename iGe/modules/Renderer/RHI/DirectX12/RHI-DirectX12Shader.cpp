module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <d3d12shader.h>
    #include <d3dcompiler.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12Shader;

namespace iGe
{

// =================================================================================================
// Static Method
// =================================================================================================

DXGI_FORMAT GetFormat(const D3D12_SIGNATURE_PARAMETER_DESC& desc) {
    int count = 0;
    if (desc.Mask & 1) { count++; }
    if (desc.Mask & 2) { count++; }
    if (desc.Mask & 4) { count++; }
    if (desc.Mask & 8) { count++; }

    if (desc.ComponentType == D3D_REGISTER_COMPONENT_FLOAT32) {
        if (count == 1) { return DXGI_FORMAT_R32_FLOAT; }
        if (count == 2) { return DXGI_FORMAT_R32G32_FLOAT; }
        if (count == 3) { return DXGI_FORMAT_R32G32B32_FLOAT; }
        if (count == 4) { return DXGI_FORMAT_R32G32B32A32_FLOAT; }
    } else if (desc.ComponentType == D3D_REGISTER_COMPONENT_SINT32) {
        if (count == 1) { return DXGI_FORMAT_R32_SINT; }
        if (count == 2) { return DXGI_FORMAT_R32G32_SINT; }
        if (count == 3) { return DXGI_FORMAT_R32G32B32_SINT; }
        if (count == 4) { return DXGI_FORMAT_R32G32B32A32_SINT; }
    } else if (desc.ComponentType == D3D_REGISTER_COMPONENT_UINT32) {
        if (count == 1) { return DXGI_FORMAT_R32_UINT; }
        if (count == 2) { return DXGI_FORMAT_R32G32_UINT; }
        if (count == 3) { return DXGI_FORMAT_R32G32B32_UINT; }
        if (count == 4) { return DXGI_FORMAT_R32G32B32A32_UINT; }
    }
    return DXGI_FORMAT_UNKNOWN;
}

const char* GetTargetProfile(RHIShaderStage stage) {
    switch (stage) {
        case RHIShaderStage::Vertex:
            return "vs_5_1";
        case RHIShaderStage::Fragment:
            return "ps_5_1";
        case RHIShaderStage::Compute:
            return "cs_5_1";
        case RHIShaderStage::Geometry:
            return "gs_5_1";
        case RHIShaderStage::TessControl:
            return "hs_5_1";
        case RHIShaderStage::TessEvaluation:
            return "ds_5_1";
        default:
            return nullptr;
    }
}

// =================================================================================================
// DirectX12Shader
// =================================================================================================

DirectX12Shader::DirectX12Shader(const RHIShaderCreateInfo& info) : RHIShader(info) {
    if (info.SourceCode.empty()) { Internal::LogError("Shader source code is empty"); }

    const char* target = GetTargetProfile(info.Stage);
    if (!target) { Internal::LogError("Unsupported shader stage"); }

    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    #if defined(IGE_DEBUG)
    compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    #endif

    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
    HRESULT hr = D3DCompile(info.SourceCode.c_str(), info.SourceCode.length(),
                            nullptr,                           // source name
                            nullptr,                           // defines
                            D3D_COMPILE_STANDARD_FILE_INCLUDE, // includes
                            info.EntryPoint.c_str(), target, compileFlags, 0, &m_Blob, &errorBlob);

    if (FAILED(hr)) {
        std::string errorMsg = "Failed to compile shader: ";
        if (errorBlob) {
            errorMsg += static_cast<const char*>(errorBlob->GetBufferPointer());
        } else {
            errorMsg += "Unknown error";
        }
        Internal::LogError("{}", errorMsg);
    } else {
        Reflect();
    }
}

void DirectX12Shader::Reflect() {
    if (!m_Blob) { return; }

    Microsoft::WRL::ComPtr<ID3D12ShaderReflection> reflector;
    if (FAILED(D3DReflect(m_Blob->GetBufferPointer(), m_Blob->GetBufferSize(),
                          IID_PPV_ARGS(reflector.GetAddressOf())))) {
        Internal::LogError("Failed to reflect shader");
        return;
    }

    // Get shader reflection information
    D3D12_SHADER_DESC shaderDesc;
    reflector->GetDesc(&shaderDesc);

    // Parse Resources
    m_Resources.clear();
    for (UINT i = 0; i < shaderDesc.BoundResources; ++i) {
        D3D12_SHADER_INPUT_BIND_DESC bindDesc;
        reflector->GetResourceBindingDesc(i, &bindDesc);

        DirectX12ShaderResource resource;
        resource.Name = bindDesc.Name;
        resource.Register = bindDesc.BindPoint;
        resource.Space = bindDesc.Space;
        resource.Count = bindDesc.BindCount;
        resource.Type = (uint32_t) bindDesc.Type;

        m_Resources.push_back(resource);
    }

    // Parse Input Layout
    if (m_Stage == RHIShaderStage::Vertex) {
        m_InputLayout.clear();
        for (UINT i = 0; i < shaderDesc.InputParameters; ++i) {
            D3D12_SIGNATURE_PARAMETER_DESC paramDesc;
            reflector->GetInputParameterDesc(i, &paramDesc);

            DirectX12InputElement elem = {};
            elem.SemanticName = paramDesc.SemanticName;
            elem.SemanticIndex = paramDesc.SemanticIndex;
            elem.Format = GetFormat(paramDesc);
            elem.InputSlot = 0;
            elem.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
            elem.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            elem.InstanceDataStepRate = 0;

            m_InputLayout.push_back(elem);
        }
    }
}

} // namespace iGe
#endif
