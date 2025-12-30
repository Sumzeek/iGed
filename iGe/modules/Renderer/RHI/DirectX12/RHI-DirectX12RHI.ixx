module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #define NOMINMAX
    #include <d3d12.h>
    #include <dxgi1_4.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12RHI;
import :RHI;
import :DirectX12Queue;
import :DirectX12DescriptorHeap;
import iGe.Common;

namespace iGe
{

export class IGE_API DirectX12RHI : public RHI {
public:
    DirectX12RHI();
    virtual ~DirectX12RHI();

    // =============================================================================
    // Device Operations
    // =============================================================================

    void WaitIdle() override;

    const RHIDeviceProperties& GetDeviceProperties() const override { return m_DeviceProperties; }
    const RHIMemoryProperties& GetMemoryProperties() const override { return m_MemoryProperties; }
    RHIFormatProperties GetFormatProperties(RHIFormat format) const override;

    // =============================================================================
    // Queue Operations
    // =============================================================================

    RHIQueue* GetQueue(RHIQueueType type, uint32 index = 0) override;
    uint32 GetQueueCount(RHIQueueType type) const override;

    // =============================================================================
    // Surface and SwapChain
    // =============================================================================

    Scope<RHISurface> CreateSurface(const RHISurfaceCreateInfo& info) override;
    Scope<RHISwapChain> CreateSwapChain(const RHISwapChainCreateInfo& info) override;

    // =============================================================================
    // Command Infrastructure
    // =============================================================================

    Scope<RHICommandPool> CreateCommandPool(const RHICommandPoolCreateInfo& info) override;

    // Allocate command list from a pool
    Scope<RHICommandList> AllocateCommandList(RHICommandPool* pPool) override;
    std::vector<Scope<RHICommandList>> AllocateCommandLists(RHICommandPool* pPool, uint32 count) override;

    // Free command lists explicitly
    void FreeCommandList(RHICommandPool* pPool, RHICommandList* pCommandList) override;
    void FreeCommandLists(RHICommandPool* pPool, std::span<RHICommandList*> commandLists) override;

    // =============================================================================
    // Buffer Operations
    // =============================================================================

    Scope<RHIBuffer> CreateBuffer(const RHIBufferCreateInfo& info) override;
    Scope<RHIVertexBuffer> CreateVertexBuffer(const RHIVertexBufferCreateInfo& info) override;
    Scope<RHIIndexBuffer> CreateIndexBuffer(const RHIIndexBufferCreateInfo& info) override;
    Scope<RHIUniformBuffer> CreateUniformBuffer(const RHIUniformBufferCreateInfo& info) override;
    Scope<RHIStorageBuffer> CreateStorageBuffer(const RHIStorageBufferCreateInfo& info) override;

    // =============================================================================
    // Texture Operations
    // =============================================================================

    Scope<RHITexture> CreateTexture(const RHITextureCreateInfo& info) override;
    Scope<RHITextureView> CreateTextureView(const RHITexture* pTexture, const RHITextureViewCreateInfo& info) override;

    // =============================================================================
    // Sampler Operations
    // =============================================================================

    Scope<RHISampler> CreateSampler(const RHISamplerCreateInfo& info) override;

    // =============================================================================
    // Descriptor Operations
    // =============================================================================

    Scope<RHIDescriptorSetLayout> CreateDescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo& info) override;
    Scope<RHIDescriptorPool> CreateDescriptorPool(const RHIDescriptorPoolCreateInfo& info) override;
    void UpdateDescriptorSets(std::span<const RHIWriteDescriptorSet> writes) override;
    void CopyDescriptorSets(std::span<const RHICopyDescriptorSet> copies) override;

    // =============================================================================
    // Pipeline Layout
    // =============================================================================

    Scope<RHIPipelineLayout> CreatePipelineLayout(const RHIPipelineLayoutCreateInfo& info) override;

    // =============================================================================
    // Render Pass and Framebuffer
    // =============================================================================

    Scope<RHIRenderPass> CreateRenderPass(const RHIRenderPassCreateInfo& info) override;
    Scope<RHIFramebuffer> CreateFramebuffer(const RHIFramebufferCreateInfo& info) override;

    // =============================================================================
    // Shader Operations
    // =============================================================================

    Scope<RHIShader> CreateShader(const RHIShaderCreateInfo& info) override;

    // =============================================================================
    // Pipeline Operations
    // =============================================================================

    Scope<RHIGraphicsPipeline> CreateGraphicsPipeline(const RHIGraphicsPipelineCreateInfo& info) override;
    Scope<RHIComputePipeline> CreateComputePipeline(const RHIComputePipelineCreateInfo& info) override;

    // =============================================================================
    // Synchronization Primitives
    // =============================================================================

    Scope<RHIFence> CreateGPUFence(const RHIFenceCreateInfo& info) override;
    Scope<RHISemaphore> CreateGPUSemaphore() override;

    bool WaitForFences(std::span<RHIFence* const> fences, bool waitAll = true,
                       uint64 timeout = std::numeric_limits<uint64>::max()) override;
    void ResetFences(std::span<RHIFence* const> fences) override;

    // =============================================================================
    // Resource Destruction
    // =============================================================================

    void DestroyResource(RHIResource* pResource) override;

    // =============================================================================
    // DirectX12 Specific
    // =============================================================================

    ID3D12Device* GetD3D12Device() const { return m_Device.Get(); }
    IDXGIFactory4* GetDXGIFactory() const { return m_Factory.Get(); }

    // Get staging descriptor heaps for CPU-side descriptor creation
    DirectX12StagingDescriptorHeap& GetCBVSRVUAVStagingHeap() { return m_CBVSRVUAVStagingHeap; }
    DirectX12StagingDescriptorHeap& GetSamplerStagingHeap() { return m_SamplerStagingHeap; }
    DirectX12StagingDescriptorHeap& GetRTVStagingHeap() { return m_RTVStagingHeap; }
    DirectX12StagingDescriptorHeap& GetDSVStagingHeap() { return m_DSVStagingHeap; }

private:
    void Init();
    void InitDeviceProperties();
    void InitStagingHeaps();

    Microsoft::WRL::ComPtr<ID3D12Device> m_Device;
    Microsoft::WRL::ComPtr<ID3D12Device5> m_Device5; // For ray tracing support
    Microsoft::WRL::ComPtr<IDXGIFactory4> m_Factory;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> m_Adapter;

    Scope<DirectX12Queue> m_GraphicsQueue = nullptr;
    Scope<DirectX12Queue> m_ComputeQueue = nullptr;
    Scope<DirectX12Queue> m_TransferQueue = nullptr;

    // Staging descriptor heaps (non-shader visible, for CPU-side descriptor creation)
    DirectX12StagingDescriptorHeap m_CBVSRVUAVStagingHeap;
    DirectX12StagingDescriptorHeap m_SamplerStagingHeap;
    DirectX12StagingDescriptorHeap m_RTVStagingHeap;
    DirectX12StagingDescriptorHeap m_DSVStagingHeap;

    // Device properties
    RHIDeviceProperties m_DeviceProperties;
    RHIMemoryProperties m_MemoryProperties;

    // Ray tracing support
    bool m_RayTracingSupported = false;

    DWORD m_CallbackCookie = 0;
};

} // namespace iGe
#endif
