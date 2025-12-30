module;
#if defined(IGE_PLATFORM_WINDOWS)
    #define NOMINMAX
    #include <d3d12.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12Descriptor;
import :DirectX12Buffer;
import :DirectX12Texture;
import :DirectX12TextureView;
import :DirectX12Sampler;
import :DirectX12Helper;

namespace iGe
{

// =================================================================================================
// DirectX12DescriptorSetLayout Implementation
// =================================================================================================

DirectX12DescriptorSetLayout::DirectX12DescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo& info)
    : RHIDescriptorSetLayout(info) {
    // Copy bindings to owned storage
    if (!info.Bindings.empty()) { m_Bindings.assign(info.Bindings.begin(), info.Bindings.end()); }

    // Count descriptors by type
    for (const auto& binding: m_Bindings) {
        if (binding.DescriptorType == RHIDescriptorType::Sampler) {
            m_SamplerCount += binding.DescriptorCount;
        } else {
            m_CBVSRVUAVCount += binding.DescriptorCount;
        }
    }
}

// =================================================================================================
// DirectX12DescriptorPool Implementation
// =================================================================================================

DirectX12DescriptorPool::DirectX12DescriptorPool(ID3D12Device* device, const RHIDescriptorPoolCreateInfo& info)
    : RHIDescriptorPool(info), m_AllowFree(info.AllowFreeDescriptorSet) {
    // Calculate total descriptors needed
    uint32 totalCBVSRVUAV = 0;
    uint32 totalSamplers = 0;

    for (const auto& poolSize: info.PoolSizes) {
        if (poolSize.Type == RHIDescriptorType::Sampler) {
            totalSamplers += poolSize.DescriptorCount;
        } else {
            totalCBVSRVUAV += poolSize.DescriptorCount;
        }
    }

    // Ensure minimum sizes
    if (totalCBVSRVUAV == 0) totalCBVSRVUAV = 1024;
    if (totalSamplers == 0) totalSamplers = 256;

    // Initialize heaps (shader-visible)
    m_CBVSRVUAVHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, totalCBVSRVUAV, true);
    m_SamplerHeap.Initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, totalSamplers, true);
}

DirectX12DescriptorPool::~DirectX12DescriptorPool() {}

void DirectX12DescriptorPool::Reset() {
    m_CBVSRVUAVHeap.Reset();
    m_SamplerHeap.Reset();
}

Scope<RHIDescriptorSet> DirectX12DescriptorPool::AllocateDescriptorSet(const RHIDescriptorSetLayout* pLayout) {
    if (!pLayout) { return nullptr; }

    auto* dx12Layout = static_cast<const DirectX12DescriptorSetLayout*>(pLayout);
    return CreateScope<DirectX12DescriptorSet>(this, dx12Layout);
}

std::vector<Scope<RHIDescriptorSet>>
DirectX12DescriptorPool::AllocateDescriptorSets(std::span<const RHIDescriptorSetLayout* const> layouts) {
    std::vector<Scope<RHIDescriptorSet>> result;
    result.reserve(layouts.size());

    for (const auto* layout: layouts) { result.push_back(AllocateDescriptorSet(layout)); }

    return result;
}

void DirectX12DescriptorPool::FreeDescriptorSet(RHIDescriptorSet* pSet) {
    if (!pSet || !m_AllowFree) { return; }

    auto* dx12Set = static_cast<DirectX12DescriptorSet*>(pSet);

    // Release resources back to pool
    dx12Set->Release();

    delete dx12Set;
}

void DirectX12DescriptorPool::FreeDescriptorSets(std::span<RHIDescriptorSet*> sets) {
    if (sets.empty()) return;

    for (auto* set: sets) { FreeDescriptorSet(set); }
}

bool DirectX12DescriptorPool::AllocateDescriptorRange(uint32 cbvSrvUavCount, uint32 samplerCount,
                                                      uint32& outCBVSRVUAVStartIndex, uint32& outSamplerStartIndex) {
    outCBVSRVUAVStartIndex = std::numeric_limits<uint32>::max();
    outSamplerStartIndex = std::numeric_limits<uint32>::max();

    if (cbvSrvUavCount > 0) {
        outCBVSRVUAVStartIndex = m_CBVSRVUAVHeap.AllocateRange(cbvSrvUavCount);
        if (outCBVSRVUAVStartIndex == std::numeric_limits<uint32>::max()) { return false; }
    }

    if (samplerCount > 0) {
        outSamplerStartIndex = m_SamplerHeap.AllocateRange(samplerCount);
        if (outSamplerStartIndex == std::numeric_limits<uint32>::max()) {
            // Rollback CBV/SRV/UAV allocation
            if (cbvSrvUavCount > 0) { m_CBVSRVUAVHeap.FreeRange(outCBVSRVUAVStartIndex, cbvSrvUavCount); }
            return false;
        }
    }

    return true;
}

void DirectX12DescriptorPool::FreeDescriptorRange(uint32 cbvSrvUavStartIndex, uint32 cbvSrvUavCount,
                                                  uint32 samplerStartIndex, uint32 samplerCount) {
    if (cbvSrvUavCount > 0 && cbvSrvUavStartIndex != std::numeric_limits<uint32>::max()) {
        m_CBVSRVUAVHeap.FreeRange(cbvSrvUavStartIndex, cbvSrvUavCount);
    }
    if (samplerCount > 0 && samplerStartIndex != std::numeric_limits<uint32>::max()) {
        m_SamplerHeap.FreeRange(samplerStartIndex, samplerCount);
    }
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectX12DescriptorPool::GetCBVSRVUAVGPUHandle(uint32 index) const {
    return m_CBVSRVUAVHeap.GetGPUHandle(index);
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectX12DescriptorPool::GetCBVSRVUAVCPUHandle(uint32 index) const {
    return m_CBVSRVUAVHeap.GetCPUHandle(index);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectX12DescriptorPool::GetSamplerGPUHandle(uint32 index) const {
    return m_SamplerHeap.GetGPUHandle(index);
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectX12DescriptorPool::GetSamplerCPUHandle(uint32 index) const {
    return m_SamplerHeap.GetCPUHandle(index);
}

// =================================================================================================
// DirectX12DescriptorSet Implementation
// =================================================================================================

DirectX12DescriptorSet::DirectX12DescriptorSet(DirectX12DescriptorPool* pool,
                                               const DirectX12DescriptorSetLayout* layout)
    : m_Pool(pool), m_Layout(layout) {
    if (!layout) { return; }

    m_CBVSRVUAVCount = layout->GetCBVSRVUAVCount();
    m_SamplerCount = layout->GetSamplerCount();

    // Allocate descriptor ranges from pool
    bool success =
            pool->AllocateDescriptorRange(m_CBVSRVUAVCount, m_SamplerCount, m_CBVSRVUAVStartIndex, m_SamplerStartIndex);
    if (!success) { Internal::LogError("Failed to allocate descriptor set from pool"); }
}

DirectX12DescriptorSet::~DirectX12DescriptorSet() { Release(); }

void DirectX12DescriptorSet::Release() {
    if (m_Pool && (m_CBVSRVUAVCount > 0 || m_SamplerCount > 0)) {
        m_Pool->FreeDescriptorRange(m_CBVSRVUAVStartIndex, m_CBVSRVUAVCount, m_SamplerStartIndex, m_SamplerCount);
        m_CBVSRVUAVStartIndex = std::numeric_limits<uint32>::max();
        m_SamplerStartIndex = std::numeric_limits<uint32>::max();
        m_CBVSRVUAVCount = 0;
        m_SamplerCount = 0;
    }
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectX12DescriptorSet::GetCBVSRVUAVTableGPUHandle() const {
    if (m_CBVSRVUAVStartIndex == std::numeric_limits<uint32>::max() || !m_Pool) { return {}; }
    return m_Pool->GetCBVSRVUAVGPUHandle(m_CBVSRVUAVStartIndex);
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectX12DescriptorSet::GetSamplerTableGPUHandle() const {
    if (m_SamplerStartIndex == std::numeric_limits<uint32>::max() || !m_Pool) { return {}; }
    return m_Pool->GetSamplerGPUHandle(m_SamplerStartIndex);
}

// =================================================================================================
// DirectX12PipelineLayout
// =================================================================================================

DirectX12PipelineLayout::DirectX12PipelineLayout(ID3D12Device* device, const RHIPipelineLayoutCreateInfo& info)
    : RHIPipelineLayout(info) {
    BuildRootSignature(device, info);
}

int32 DirectX12PipelineLayout::GetRootParameterIndex(uint32 setIndex, uint32 bindingIndex) const {
    for (const auto& mapping: m_RootParameterMappings) {
        if (mapping.SetIndex == setIndex && mapping.BindingIndex == bindingIndex) {
            return static_cast<int32>(mapping.RootParameterIndex);
        }
    }
    return -1;
}

void DirectX12PipelineLayout::BuildRootSignature(ID3D12Device* device, const RHIPipelineLayoutCreateInfo& info) {
    if (!device) { Internal::LogError("DirectX12PipelineLayout: Device is null"); }

    std::vector<D3D12_ROOT_PARAMETER1> rootParameters;
    std::vector<std::vector<D3D12_DESCRIPTOR_RANGE1>> descriptorRangeStorage; // Keep ranges alive

    uint32 rootParameterIndex = 0;

    // Collect all CBV/SRV/UAV ranges across all sets into a single table
    // Collect all Sampler ranges across all sets into another table
    // This matches the BindDescriptorSets logic: param 0 = CBV_SRV_UAV, param 1 = Sampler
    std::vector<D3D12_DESCRIPTOR_RANGE1> allCbvSrvUavRanges;
    std::vector<D3D12_DESCRIPTOR_RANGE1> allSamplerRanges;

    // Per-register-type counters for D3D12 shader register assignment
    // D3D12 uses separate register namespaces: b (CBV), t (SRV), u (UAV), s (Sampler)
    // Vulkan uses a single binding namespace, so we need to remap
    uint32 cbvRegisterCounter = 0;
    uint32 srvRegisterCounter = 0;
    uint32 uavRegisterCounter = 0;
    uint32 samplerRegisterCounter = 0;

    // Process each descriptor set layout
    for (uint32 setIndex = 0; setIndex < info.SetLayouts.size(); ++setIndex) {
        const auto* dx12SetLayout = static_cast<const DirectX12DescriptorSetLayout*>(info.SetLayouts[setIndex]);
        if (!dx12SetLayout) { continue; }

        const auto& bindings = dx12SetLayout->GetBindings();

        for (const auto& binding: bindings) {
            D3D12_DESCRIPTOR_RANGE1 range = {};
            range.RangeType = GetDescriptorRangeType(binding.DescriptorType);
            range.NumDescriptors = binding.DescriptorCount;
            range.RegisterSpace = setIndex;
            range.Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
            range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            // Assign BaseShaderRegister based on register type, not Vulkan binding index
            // This allows Vulkan-style unique bindings to work with D3D12's separate namespaces
            switch (range.RangeType) {
                case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                    range.BaseShaderRegister = cbvRegisterCounter;
                    cbvRegisterCounter += binding.DescriptorCount;
                    break;
                case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                    range.BaseShaderRegister = srvRegisterCounter;
                    srvRegisterCounter += binding.DescriptorCount;
                    break;
                case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                    range.BaseShaderRegister = uavRegisterCounter;
                    uavRegisterCounter += binding.DescriptorCount;
                    break;
                case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
                    range.BaseShaderRegister = samplerRegisterCounter;
                    samplerRegisterCounter += binding.DescriptorCount;
                    break;
            }

            if (range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER) {
                allSamplerRanges.push_back(range);
            } else {
                // CBV, SRV, UAV all go to the same table
                allCbvSrvUavRanges.push_back(range);
            }
        }
    }

    // Create CBV/SRV/UAV descriptor table as parameter 0
    if (!allCbvSrvUavRanges.empty()) {
        descriptorRangeStorage.push_back(std::move(allCbvSrvUavRanges));
        auto& storedRanges = descriptorRangeStorage.back();

        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(storedRanges.size());
        param.DescriptorTable.pDescriptorRanges = storedRanges.data();
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        rootParameters.push_back(param);

        // Store mappings for all sets - CBV/SRV/UAV bindings map to parameter 0
        for (uint32 setIndex = 0; setIndex < info.SetLayouts.size(); ++setIndex) {
            DirectX12RootParameterMapping mapping;
            mapping.SetIndex = setIndex;
            mapping.BindingIndex = 0; // CBV_SRV_UAV table
            mapping.RootParameterIndex = rootParameterIndex;
            mapping.Type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            m_RootParameterMappings.push_back(mapping);
        }
        ++rootParameterIndex;
    }

    // Create Sampler descriptor table as parameter 1
    if (!allSamplerRanges.empty()) {
        descriptorRangeStorage.push_back(std::move(allSamplerRanges));
        auto& storedRanges = descriptorRangeStorage.back();

        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(storedRanges.size());
        param.DescriptorTable.pDescriptorRanges = storedRanges.data();
        param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        rootParameters.push_back(param);

        // Store mappings for all sets - Sampler bindings map to parameter 1
        for (uint32 setIndex = 0; setIndex < info.SetLayouts.size(); ++setIndex) {
            DirectX12RootParameterMapping mapping;
            mapping.SetIndex = setIndex;
            mapping.BindingIndex = 1; // Sampler table
            mapping.RootParameterIndex = rootParameterIndex;
            mapping.Type = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            m_RootParameterMappings.push_back(mapping);
        }
        ++rootParameterIndex;
    }

    // Add push constants as root constants
    for (const auto& pushConstant: info.PushConstantRanges) {
        D3D12_ROOT_PARAMETER1 param = {};
        param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        param.Constants.ShaderRegister = 0; // b0 typically
        param.Constants.RegisterSpace = 0;
        param.Constants.Num32BitValues = pushConstant.Size / 4;
        param.ShaderVisibility = GetShaderVisibility(pushConstant.StageFlags);

        m_PushConstantRootIndex = static_cast<int32>(rootParameterIndex);
        rootParameters.push_back(param);
        ++rootParameterIndex;
    }

    // Define static samplers (optional, for common samplers)
    std::vector<D3D12_STATIC_SAMPLER_DESC> staticSamplers;

    // Add common static samplers
    D3D12_STATIC_SAMPLER_DESC linearWrapSampler = {};
    linearWrapSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    linearWrapSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrapSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrapSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    linearWrapSampler.MipLODBias = 0;
    linearWrapSampler.MaxAnisotropy = 1;
    linearWrapSampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    linearWrapSampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    linearWrapSampler.MinLOD = 0.0f;
    linearWrapSampler.MaxLOD = D3D12_FLOAT32_MAX;
    linearWrapSampler.ShaderRegister = 0;
    linearWrapSampler.RegisterSpace = 100; // High space to avoid conflicts
    linearWrapSampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    staticSamplers.push_back(linearWrapSampler);

    D3D12_STATIC_SAMPLER_DESC pointClampSampler = linearWrapSampler;
    pointClampSampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    pointClampSampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    pointClampSampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    pointClampSampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    pointClampSampler.ShaderRegister = 1;
    staticSamplers.push_back(pointClampSampler);

    // Build root signature description
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootSigDesc.Desc_1_1.NumParameters = static_cast<UINT>(rootParameters.size());
    rootSigDesc.Desc_1_1.pParameters = rootParameters.empty() ? nullptr : rootParameters.data();
    rootSigDesc.Desc_1_1.NumStaticSamplers = static_cast<UINT>(staticSamplers.size());
    rootSigDesc.Desc_1_1.pStaticSamplers = staticSamplers.empty() ? nullptr : staticSamplers.data();
    rootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // Serialize root signature
    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeVersionedRootSignature(&rootSigDesc, &signatureBlob, &errorBlob);
    if (FAILED(hr)) {
        std::string errorMsg = "DirectX12PipelineLayout: Failed to serialize root signature";
        if (errorBlob) {
            errorMsg += ": ";
            errorMsg += static_cast<const char*>(errorBlob->GetBufferPointer());
        }
        Internal::LogError("{}", errorMsg);
    }

    // Create root signature
    hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&m_RootSignature));

    if (FAILED(hr)) { Internal::LogError("DirectX12PipelineLayout: Failed to create root signature"); }
}

D3D12_DESCRIPTOR_RANGE_TYPE DirectX12PipelineLayout::GetDescriptorRangeType(RHIDescriptorType type) const {
    switch (type) {
        case RHIDescriptorType::UniformBuffer:
        case RHIDescriptorType::UniformBufferDynamic:
            return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;

        case RHIDescriptorType::SampledImage:
        case RHIDescriptorType::UniformTexelBuffer:
        case RHIDescriptorType::InputAttachment:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        case RHIDescriptorType::StorageBuffer:
        case RHIDescriptorType::StorageBufferDynamic:
        case RHIDescriptorType::StorageImage:
        case RHIDescriptorType::StorageTexelBuffer:
            return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;

        case RHIDescriptorType::Sampler:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;

        case RHIDescriptorType::CombinedImageSampler:
            // For combined sampler, we need both SRV and Sampler
            // Return SRV here, sampler is handled separately
            return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

        default:
            return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    }
}

D3D12_SHADER_VISIBILITY DirectX12PipelineLayout::GetShaderVisibility(Flags<RHIShaderStage> stages) const {
    // If multiple stages, use ALL
    uint32 stageCount = 0;
    if (stages.HasFlag(RHIShaderStage::Vertex)) { ++stageCount; }
    if (stages.HasFlag(RHIShaderStage::Fragment)) { ++stageCount; }
    if (stages.HasFlag(RHIShaderStage::Compute)) { ++stageCount; }
    if (stages.HasFlag(RHIShaderStage::Geometry)) { ++stageCount; }
    if (stages.HasFlag(RHIShaderStage::TessControl)) { ++stageCount; }
    if (stages.HasFlag(RHIShaderStage::TessEvaluation)) { ++stageCount; }

    if (stageCount > 1) { return D3D12_SHADER_VISIBILITY_ALL; }

    if (stages.HasFlag(RHIShaderStage::Vertex)) { return D3D12_SHADER_VISIBILITY_VERTEX; }
    if (stages.HasFlag(RHIShaderStage::Fragment)) { return D3D12_SHADER_VISIBILITY_PIXEL; }
    if (stages.HasFlag(RHIShaderStage::Geometry)) { return D3D12_SHADER_VISIBILITY_GEOMETRY; }
    if (stages.HasFlag(RHIShaderStage::TessControl)) { return D3D12_SHADER_VISIBILITY_HULL; }
    if (stages.HasFlag(RHIShaderStage::TessEvaluation)) { return D3D12_SHADER_VISIBILITY_DOMAIN; }

    return D3D12_SHADER_VISIBILITY_ALL;
}

} // namespace iGe
#endif
