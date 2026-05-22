module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <d3dcompiler.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12Shader;

namespace iGe
{

// =================================================================================================
// Static Method
// =================================================================================================

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
    if (!info.Bytecode.empty()) {
        // Pre-compiled bytecode path: directly create blob from bytecode
        HRESULT hr = D3DCreateBlob(info.Bytecode.size(), &m_Blob);
        if (FAILED(hr)) {
            Internal::LogError("Failed to create D3D blob from bytecode");
            return;
        }
        memcpy(m_Blob->GetBufferPointer(), info.Bytecode.data(), info.Bytecode.size());

    } else if (!info.SourceCode.empty()) {
        // Runtime compilation fallback: compile HLSL source with FXC
        const char* target = GetTargetProfile(info.Stage);
        if (!target) {
            Internal::LogError("Unsupported shader stage");
            return;
        }

        UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;
        #if defined(IGE_DEBUG)
        compileFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        #endif

        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;
        HRESULT hr = D3DCompile(info.SourceCode.c_str(), info.SourceCode.length(),
                                nullptr, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                                info.EntryPoint.c_str(), target, compileFlags, 0,
                                &m_Blob, &errorBlob);

        if (FAILED(hr)) {
            std::string errorMsg = "Failed to compile shader: ";
            if (errorBlob) {
                errorMsg += static_cast<const char*>(errorBlob->GetBufferPointer());
            } else {
                errorMsg += "Unknown error";
            }
            Internal::LogError("{}", errorMsg);
        }

    } else {
        Internal::LogError("Shader has neither bytecode nor source code");
    }
}

} // namespace iGe
#endif
