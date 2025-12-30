module;
#include "iGeMacro.h"

export module iGe.RHI:RHIResource;
import iGe.Common;

namespace iGe
{

export enum class RHIMemoryUsage : uint32 {
    Unknown = 0,
    GpuOnly,           // Fast GPU memory, not accessible from CPU
    CpuOnly,           // Staging buffers, CPU-side
    CpuToGpu,          // Upload heaps (write from CPU, read from GPU)
    GpuToCpu,          // Readback heaps (write from GPU, read from CPU)
    CpuCopy,           // For CPU-to-CPU copy
    GpuLazilyAllocated // Lazily allocated, for transient attachments
};

export enum class RHIResourceType : uint32 {
    // Buffer
    Buffer = 0,

    // Textures and Views
    Texture,
    TextureView,
    Sampler,

    // Shaders
    Shader,
    ShaderBindingTable,

    // Descriptors and Layouts
    PipelineLayout, // Vulkan Pipeline Layout
    DescriptorSetLayout,
    DescriptorPool,
    DescriptorSet,

    // Render Pass and Framebuffer
    RenderPass,
    Framebuffer,

    // Pipelines
    GraphicsPipeline,
    ComputePipeline,

    // Command Infrastructure
    Queue,
    CommandPool,
    CommandList,

    // Synchronization
    Fence,
    Semaphore,

    // Presentation
    Surface,
    SwapChain,

    Count
};

export class IGE_API RHIResource {
public:
    RHIResource() = delete;
    virtual ~RHIResource() = default;

    // Type identification
    inline RHIResourceType GetResourceType() const { return m_Type; }

    // Native handle access
    virtual void* GetNativeHandle() const { return nullptr; }

protected:
    RHIResource(RHIResourceType type) : m_Type(type) {}

    RHIResourceType m_Type;
};

} // namespace iGe
