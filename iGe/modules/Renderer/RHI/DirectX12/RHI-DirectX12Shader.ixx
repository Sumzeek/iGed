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
// DirectX12ShaderResource
// =================================================================================================

export struct DirectX12ShaderResource {
    std::string Name;
    uint32 Register = 0;
    uint32 Space = 0;
    uint32 Count = 1;
    uint32 Type = 0; // D3D_SHADER_INPUT_TYPE
};

// =================================================================================================
// DirectX12InputElement
// =================================================================================================

export struct DirectX12InputElement {
    std::string SemanticName;
    uint32 SemanticIndex = 0;
    DXGI_FORMAT Format = DXGI_FORMAT_UNKNOWN;
    uint32 InputSlot = 0;
    uint32 AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
    D3D12_INPUT_CLASSIFICATION InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    uint32 InstanceDataStepRate = 0;
};

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

    // Reflection data
    const std::vector<DirectX12ShaderResource>& GetResources() const { return m_Resources; }
    const std::vector<DirectX12InputElement>& GetInputLayout() const { return m_InputLayout; }

private:
    void Reflect();

    Microsoft::WRL::ComPtr<ID3DBlob> m_Blob;
    std::vector<DirectX12ShaderResource> m_Resources;
    std::vector<DirectX12InputElement> m_InputLayout;
    RHIShaderStage m_Stage = RHIShaderStage::Vertex;
    std::string m_EntryPoint = "main";
    std::vector<uint8> m_Bytecode;
    bool m_UseDXC = true; // Prefer DXC for SM6.0+
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

    // Compile HLSL to DXBC (using FXC, for older shader models)
    bool CompileLegacyHLSL(const std::string& source, const std::string& entryPoint, const std::string& target,
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
