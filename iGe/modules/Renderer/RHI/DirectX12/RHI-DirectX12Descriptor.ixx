module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <vector>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12Descriptor;
import :RHIDescriptor;
import :DirectX12DescriptorHeap;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// DirectX12 Descriptor Set Layout
// =================================================================================================

export class IGE_API DirectX12DescriptorSetLayout : public RHIDescriptorSetLayout {
public:
    DirectX12DescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo& info);
    ~DirectX12DescriptorSetLayout() override = default;

    void* GetNativeHandle() const override { return nullptr; }

    // Get the total number of CBV/SRV/UAV descriptors
    uint32 GetCBVSRVUAVCount() const { return m_CBVSRVUAVCount; }

    // Get the total number of sampler descriptors
    uint32 GetSamplerCount() const { return m_SamplerCount; }

    // Get stored bindings
    const std::vector<RHIDescriptorSetLayoutBinding>& GetBindings() const { return m_Bindings; }

private:
    std::vector<RHIDescriptorSetLayoutBinding> m_Bindings;
    uint32 m_CBVSRVUAVCount = 0;
    uint32 m_SamplerCount = 0;
};

// Forward declaration
export class DirectX12DescriptorSet;

// =================================================================================================
// DirectX12 Descriptor Pool
// =================================================================================================

export class IGE_API DirectX12DescriptorPool : public RHIDescriptorPool {
public:
    DirectX12DescriptorPool(ID3D12Device* device, const RHIDescriptorPoolCreateInfo& info);
    ~DirectX12DescriptorPool() override;

    void* GetNativeHandle() const override { return nullptr; }
    void Reset() override;

    // Override base class allocation methods
    Scope<RHIDescriptorSet> AllocateDescriptorSet(const RHIDescriptorSetLayout* pLayout) override;
    std::vector<Scope<RHIDescriptorSet>>
    AllocateDescriptorSets(std::span<const RHIDescriptorSetLayout* const> layouts) override;
    void FreeDescriptorSet(RHIDescriptorSet* pSet) override;
    void FreeDescriptorSets(std::span<RHIDescriptorSet*> sets) override;

    // Internal allocation helpers
    bool AllocateDescriptorRange(uint32 cbvSrvUavCount, uint32 samplerCount, uint32& outCBVSRVUAVStartIndex,
                                 uint32& outSamplerStartIndex);
    void FreeDescriptorRange(uint32 cbvSrvUavStartIndex, uint32 cbvSrvUavCount, uint32 samplerStartIndex,
                             uint32 samplerCount);

    // Get GPU-visible heap handles
    D3D12_GPU_DESCRIPTOR_HANDLE GetCBVSRVUAVGPUHandle(uint32 index) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetCBVSRVUAVCPUHandle(uint32 index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerGPUHandle(uint32 index) const;
    D3D12_CPU_DESCRIPTOR_HANDLE GetSamplerCPUHandle(uint32 index) const;

    // Get heaps
    ID3D12DescriptorHeap* GetCBVSRVUAVHeap() const { return m_CBVSRVUAVHeap.GetHeap(); }
    ID3D12DescriptorHeap* GetSamplerHeap() const { return m_SamplerHeap.GetHeap(); }

    uint32 GetCBVSRVUAVDescriptorSize() const { return m_CBVSRVUAVHeap.GetDescriptorSize(); }
    uint32 GetSamplerDescriptorSize() const { return m_SamplerHeap.GetDescriptorSize(); }

private:
    DirectX12DescriptorHeapAllocator m_CBVSRVUAVHeap;
    DirectX12DescriptorHeapAllocator m_SamplerHeap;

    bool m_AllowFree = false;
};

// =================================================================================================
// DirectX12 Descriptor Set
// =================================================================================================

export class IGE_API DirectX12DescriptorSet : public RHIDescriptorSet {
public:
    DirectX12DescriptorSet(DirectX12DescriptorPool* pool, const DirectX12DescriptorSetLayout* layout);
    ~DirectX12DescriptorSet() override;

    void* GetNativeHandle() const override { return nullptr; }

    // Get GPU handles for binding
    D3D12_GPU_DESCRIPTOR_HANDLE GetCBVSRVUAVTableGPUHandle() const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetSamplerTableGPUHandle() const;

    // Check if the set has descriptors of each type
    bool HasCBVSRVUAV() const { return m_CBVSRVUAVCount > 0; }
    bool HasSamplers() const { return m_SamplerCount > 0; }

    // Get the pool to access descriptor heaps
    DirectX12DescriptorPool* GetPool() const { return m_Pool; }

    // Get the layout
    const DirectX12DescriptorSetLayout* GetLayout() const { return m_Layout; }

    // Get start indices for internal use
    uint32 GetCBVSRVUAVStartIndex() const { return m_CBVSRVUAVStartIndex; }
    uint32 GetSamplerStartIndex() const { return m_SamplerStartIndex; }

    // Free resources back to pool
    void Release();

private:
    friend class DirectX12RHI; // Allow RHI to access internal members for descriptor updates

    DirectX12DescriptorPool* m_Pool = nullptr;
    const DirectX12DescriptorSetLayout* m_Layout = nullptr;

    uint32 m_CBVSRVUAVStartIndex = std::numeric_limits<uint32>::max();
    uint32 m_SamplerStartIndex = std::numeric_limits<uint32>::max();
    uint32 m_CBVSRVUAVCount = 0;
    uint32 m_SamplerCount = 0;
};

// =================================================================================================
// DirectX12PipelineLayout (Root Signature)
// =================================================================================================

// Mapping from RHI descriptor set/binding to D3D12 root parameter
struct DirectX12RootParameterMapping {
    uint32 SetIndex;
    uint32 BindingIndex;
    uint32 RootParameterIndex;
    D3D12_ROOT_PARAMETER_TYPE Type;
};

export class IGE_API DirectX12PipelineLayout : public RHIPipelineLayout {
public:
    DirectX12PipelineLayout(ID3D12Device* device, const RHIPipelineLayoutCreateInfo& info);
    ~DirectX12PipelineLayout() override = default;

    // Get root signature for PSO creation
    ID3D12RootSignature* GetRootSignature() const { return m_RootSignature.Get(); }

    // Get root parameter index for a specific descriptor set/binding
    int32 GetRootParameterIndex(uint32 setIndex, uint32 bindingIndex) const;

    // Get push constant root parameter index
    int32 GetPushConstantRootIndex() const { return m_PushConstantRootIndex; }

    // Get the total number of root parameters
    uint32 GetRootParameterCount() const { return static_cast<uint32>(m_RootParameterMappings.size()); }

    void* GetNativeHandle() const override { return m_RootSignature.Get(); }

private:
    void BuildRootSignature(ID3D12Device* device, const RHIPipelineLayoutCreateInfo& info);

    D3D12_DESCRIPTOR_RANGE_TYPE GetDescriptorRangeType(RHIDescriptorType type) const;
    D3D12_SHADER_VISIBILITY GetShaderVisibility(Flags<RHIShaderStage> stages) const;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
    std::vector<DirectX12RootParameterMapping> m_RootParameterMappings;
    int32 m_PushConstantRootIndex = -1;
};

} // namespace iGe
#endif
