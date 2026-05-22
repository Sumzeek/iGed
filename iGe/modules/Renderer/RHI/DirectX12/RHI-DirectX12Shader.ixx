module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <d3dcompiler.h>
    #include <dxcapi.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12Shader;
import :RHIShader;

namespace iGe
{

// =================================================================================================
// DirectX12Shader
// =================================================================================================

export class IGE_API DirectX12Shader : public RHIShader {
public:
    DirectX12Shader(const RHIShaderCreateInfo& info);
    ~DirectX12Shader() override = default;

    // D3D12 specific
    ID3DBlob* GetBlob() const { return m_Blob.Get(); }
    const void* GetBytecode() const { return m_Blob ? m_Blob->GetBufferPointer() : nullptr; }
    size_t GetBytecodeSize() const { return m_Blob ? m_Blob->GetBufferSize() : 0; }
    void* GetNativeHandle() const override { return m_Blob.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3DBlob> m_Blob;
};

// =================================================================================================
// DirectX12ShaderCompiler
// =================================================================================================

export class IGE_API DirectX12ShaderCompiler {
public:
    static DirectX12ShaderCompiler& Get();

    bool Initialize();
    void Shutdown();

    // Compile HLSL to DXIL (using DXC)
    bool CompileHLSL(const std::string& source, const std::string& entryPoint, const std::wstring& target,
                     std::vector<uint8>& outBytecode, std::string& outErrors);

    bool IsInitialized() const { return m_Initialized; }

private:
    DirectX12ShaderCompiler() = default;
    ~DirectX12ShaderCompiler() = default;

    Microsoft::WRL::ComPtr<IDxcUtils> m_DxcUtils;
    Microsoft::WRL::ComPtr<IDxcCompiler3> m_DxcCompiler;
    Microsoft::WRL::ComPtr<IDxcIncludeHandler> m_IncludeHandler;
    bool m_Initialized = false;
};

} // namespace iGe
#endif
