module;
#include "iGeMacro.h"

export module iGe.RHI:RHI;
import :RHIQueue;
import :RHISurface;
import :RHISwapChain;
import :RHICommandPool;
import :RHIDescriptor;
import :RHITextureView;
import :RHIFence;
import :RHISemaphore;
import :RHIFramebuffer;
import :RHIGraphicsPipeline;
import :RHIComputePipeline;
import :RHIDeviceCapabilities;
import iGe.Common;

namespace iGe
{

export enum class GraphicsAPI : uint32 { Vulkan = 0, DirectX12, Metal, Count };

export class IGE_API RHI {
public:
    struct Config {
        GraphicsAPI GraphicsAPI = GraphicsAPI::Vulkan;
        bool EnableValidation = true;
        bool EnableDebugMarkers = true;
    };

    static RHI* Init(const Config& config);
    static RHI* Get() { return s_RHI.get(); }
    static GraphicsAPI GetGraphicsAPI() { return s_Config.GraphicsAPI; }
    static void Shutdown();

    virtual ~RHI() = default;

    // =============================================================================
    // Device Operations
    // =============================================================================

    virtual void WaitIdle() = 0;

    // Get device properties and capabilities
    virtual const RHIDeviceProperties& GetDeviceProperties() const = 0;
    virtual const RHIMemoryProperties& GetMemoryProperties() const = 0;
    virtual RHIFormatProperties GetFormatProperties(RHIFormat format) const = 0;

    // =============================================================================
    // Queue Operations
    // =============================================================================

    virtual RHIQueue* GetQueue(RHIQueueType type, uint32 index = 0) = 0;
    virtual uint32 GetQueueCount(RHIQueueType type) const = 0;

    // =============================================================================
    // Surface and SwapChain
    // =============================================================================

    virtual Scope<RHISurface> CreateSurface(const RHISurfaceCreateInfo& info) = 0;
    virtual Scope<RHISwapChain> CreateSwapChain(const RHISwapChainCreateInfo& info) = 0;

    // =============================================================================
    // Command Infrastructure
    // =============================================================================

    virtual Scope<RHICommandPool> CreateCommandPool(const RHICommandPoolCreateInfo& info) = 0;

    // Allocate command list from a pool (convenience wrapper)
    virtual Scope<RHICommandList> AllocateCommandList(RHICommandPool* pPool) = 0;
    virtual std::vector<Scope<RHICommandList>> AllocateCommandLists(RHICommandPool* pPool, uint32 count) = 0;

    // Free command lists explicitly (convenience wrapper)
    // Note: Prefer letting Scope handle cleanup automatically
    virtual void FreeCommandList(RHICommandPool* pPool, RHICommandList* pCommandList) = 0;
    virtual void FreeCommandLists(RHICommandPool* pPool, std::span<RHICommandList*> commandLists) = 0;

    // =============================================================================
    // Buffer Operations
    // =============================================================================

    virtual Scope<RHIBuffer> CreateBuffer(const RHIBufferCreateInfo& info) = 0;
    virtual Scope<RHIVertexBuffer> CreateVertexBuffer(const RHIVertexBufferCreateInfo& info) = 0;
    virtual Scope<RHIIndexBuffer> CreateIndexBuffer(const RHIIndexBufferCreateInfo& info) = 0;
    virtual Scope<RHIUniformBuffer> CreateUniformBuffer(const RHIUniformBufferCreateInfo& info) = 0;
    virtual Scope<RHIStorageBuffer> CreateStorageBuffer(const RHIStorageBufferCreateInfo& info) = 0;

    // =============================================================================
    // Texture Operations
    // =============================================================================

    virtual Scope<RHITexture> CreateTexture(const RHITextureCreateInfo& info) = 0;
    virtual Scope<RHITextureView> CreateTextureView(const RHITexture* pTexture,
                                                    const RHITextureViewCreateInfo& info) = 0;

    // =============================================================================
    // Sampler Operations
    // =============================================================================

    virtual Scope<RHISampler> CreateSampler(const RHISamplerCreateInfo& info) = 0;

    // =============================================================================
    // Descriptor Operations
    // =============================================================================

    virtual Scope<RHIDescriptorSetLayout> CreateDescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo& info) = 0;
    virtual Scope<RHIDescriptorPool> CreateDescriptorPool(const RHIDescriptorPoolCreateInfo& info) = 0;

    // Update descriptor sets with writes
    virtual void UpdateDescriptorSets(std::span<const RHIWriteDescriptorSet> writes) = 0;

    // Copy descriptors between sets
    virtual void CopyDescriptorSets(std::span<const RHICopyDescriptorSet> copies) = 0;

    // =============================================================================
    // Pipeline Layout
    // =============================================================================

    virtual Scope<RHIPipelineLayout> CreatePipelineLayout(const RHIPipelineLayoutCreateInfo& info) = 0;

    // =============================================================================
    // Render Pass and Framebuffer
    // =============================================================================

    virtual Scope<RHIRenderPass> CreateRenderPass(const RHIRenderPassCreateInfo& info) = 0;
    virtual Scope<RHIFramebuffer> CreateFramebuffer(const RHIFramebufferCreateInfo& info) = 0;

    // =============================================================================
    // Shader Operations
    // =============================================================================

    virtual Scope<RHIShader> CreateShader(const RHIShaderCreateInfo& info) = 0;

    // =============================================================================
    // Pipeline Operations
    // =============================================================================

    virtual Scope<RHIGraphicsPipeline> CreateGraphicsPipeline(const RHIGraphicsPipelineCreateInfo& info) = 0;
    virtual Scope<RHIComputePipeline> CreateComputePipeline(const RHIComputePipelineCreateInfo& info) = 0;

    // =============================================================================
    // Synchronization Primitives
    // =============================================================================

    virtual Scope<RHIFence> CreateGPUFence(const RHIFenceCreateInfo& info) = 0;
    virtual Scope<RHISemaphore> CreateGPUSemaphore() = 0;

    // Wait for multiple fences
    virtual bool WaitForFences(std::span<RHIFence* const> fences, bool waitAll = true,
                               uint64 timeout = std::numeric_limits<uint64>::max()) = 0;
    virtual void ResetFences(std::span<RHIFence* const> fences) = 0;

    // =============================================================================
    // Resource Destruction
    // =============================================================================

    // Generic destroy function - will determine type at runtime
    virtual void DestroyResource(RHIResource* pResource) = 0;

    // Type-safe destroy functions (optional, can just use DestroyResource)
    virtual void DestroySurface(RHISurface* pSurface) { DestroyResource(pSurface); }
    virtual void DestroySwapChain(RHISwapChain* pSwapChain) { DestroyResource(pSwapChain); }
    virtual void DestroyCommandPool(RHICommandPool* pCommandPool) { DestroyResource(pCommandPool); }
    virtual void DestroyBuffer(RHIBuffer* pBuffer) { DestroyResource(pBuffer); }
    virtual void DestroyTexture(RHITexture* pTexture) { DestroyResource(pTexture); }
    virtual void DestroyTextureView(RHITextureView* pTextureView) { DestroyResource(pTextureView); }
    virtual void DestroySampler(RHISampler* pSampler) { DestroyResource(pSampler); }
    virtual void DestroyDescriptorSetLayout(RHIDescriptorSetLayout* pLayout) { DestroyResource(pLayout); }
    virtual void DestroyDescriptorPool(RHIDescriptorPool* pPool) { DestroyResource(pPool); }
    virtual void DestroyPipelineLayout(RHIPipelineLayout* pLayout) { DestroyResource(pLayout); }
    virtual void DestroyRenderPass(RHIRenderPass* pRenderPass) { DestroyResource(pRenderPass); }
    virtual void DestroyFramebuffer(RHIFramebuffer* pFramebuffer) { DestroyResource(pFramebuffer); }
    virtual void DestroyShader(RHIShader* pShader) { DestroyResource(pShader); }
    virtual void DestroyGraphicsPipeline(RHIGraphicsPipeline* pPipeline) { DestroyResource(pPipeline); }
    virtual void DestroyComputePipeline(RHIComputePipeline* pPipeline) { DestroyResource(pPipeline); }
    virtual void DestroyFence(RHIFence* pFence) { DestroyResource(pFence); }
    virtual void DestroySemaphore(RHISemaphore* pSemaphore) { DestroyResource(pSemaphore); }

protected:
    RHI() = default;

    inline static Config s_Config;
    inline static Scope<RHI> s_RHI = nullptr;
};

} // namespace iGe
