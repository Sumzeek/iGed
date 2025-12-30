module;
#include "iGeMacro.h"

export module iGe.RHI:RHIDescriptor;
import :RHIResource;
import :RHIBuffer;
import :RHITextureView;
import :RHISampler;
import :RHIShader;
import :RHIRenderPass;
import iGe.Common;

namespace iGe
{

export enum class RHIDescriptorType : uint32 {
    Sampler = 0,
    CombinedImageSampler,
    SampledImage,
    StorageImage,
    UniformTexelBuffer,
    StorageTexelBuffer,
    UniformBuffer,
    StorageBuffer,
    UniformBufferDynamic,
    StorageBufferDynamic,
    InputAttachment,

    Count
};

export enum class RHIDescriptorBindingFlagBits : uint32 {
    None = 0,
    UpdateAfterBind = 1 << 0,
    UpdateUnusedWhilePending = 1 << 1,
    PartiallyBound = 1 << 2,
    VariableDescriptorCount = 1 << 3
};

// =================================================================================================
// Descriptor Set Layout Binding
// =================================================================================================

export struct RHIDescriptorSetLayoutBinding {
    uint32 Binding = 0;
    RHIDescriptorType DescriptorType = RHIDescriptorType::UniformBuffer;
    uint32 DescriptorCount = 1;
    Flags<RHIShaderStage> StageFlags = RHIShaderStage::Vertex;
    Flags<RHIDescriptorBindingFlagBits> BindingFlags = RHIDescriptorBindingFlagBits::None;
};

// =================================================================================================
// Descriptor Set Layout
// =================================================================================================

export struct RHIDescriptorSetLayoutCreateInfo {
    std::span<const RHIDescriptorSetLayoutBinding> Bindings = {};
    bool UpdateAfterBindPool = false; // Allow update after bind
};

export class IGE_API RHIDescriptorSetLayout : public RHIResource {
public:
    ~RHIDescriptorSetLayout() override = default;

protected:
    RHIDescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo& info)
        : RHIResource(RHIResourceType::DescriptorSetLayout) {}
};

// =================================================================================================
// Push Constants Range
// =================================================================================================

export struct RHIPushConstantRange {
    Flags<RHIShaderStage> StageFlags;
    uint32 Offset = 0;
    uint32 Size = 0;
};

// =================================================================================================
// Pipeline Layout
// =================================================================================================

export struct RHIPipelineLayoutCreateInfo {
    std::span<const RHIDescriptorSetLayout* const> SetLayouts = {};
    std::span<const RHIPushConstantRange> PushConstantRanges = {};
};

export class IGE_API RHIPipelineLayout : public RHIResource {
public:
    ~RHIPipelineLayout() override = default;

    uint32 GetDescriptorSetLayoutCount() const { return m_SetLayoutCount; }
    uint32 GetPushConstantRangeCount() const { return m_PushConstantRangeCount; }

protected:
    RHIPipelineLayout(const RHIPipelineLayoutCreateInfo& info)
        : RHIResource(RHIResourceType::PipelineLayout), m_SetLayoutCount(static_cast<uint32>(info.SetLayouts.size())),
          m_PushConstantRangeCount(static_cast<uint32>(info.PushConstantRanges.size())) {}

    uint32 m_SetLayoutCount = 0;
    uint32 m_PushConstantRangeCount = 0;
};

// =================================================================================================
// Descriptor Pool
// =================================================================================================

export struct RHIDescriptorPoolSize {
    RHIDescriptorType Type;
    uint32 DescriptorCount;
};

export struct RHIDescriptorPoolCreateInfo {
    uint32 MaxSets = 1000;
    std::span<const RHIDescriptorPoolSize> PoolSizes = {};
    bool AllowFreeDescriptorSet = false;
    bool UpdateAfterBind = false;
};

// Forward declaration
export class RHIDescriptorSet;

export class IGE_API RHIDescriptorPool : public RHIResource {
public:
    ~RHIDescriptorPool() override = default;

    // Reset the pool, freeing all allocated descriptor sets
    virtual void Reset() = 0;

    // Allocate a single descriptor set from this pool
    // Returns ownership to the caller via Scope (unique_ptr)
    virtual Scope<RHIDescriptorSet> AllocateDescriptorSet(const RHIDescriptorSetLayout* pLayout) = 0;

    // Allocate multiple descriptor sets
    // Returns a vector of Scope-managed descriptor sets
    virtual std::vector<Scope<RHIDescriptorSet>>
    AllocateDescriptorSets(std::span<const RHIDescriptorSetLayout* const> layouts) = 0;

    // Free a descriptor set explicitly (for manual memory management)
    // Note: Prefer letting Scope handle cleanup automatically
    // Only works if AllowFreeDescriptorSet was true at creation
    virtual void FreeDescriptorSet(RHIDescriptorSet* pSet) = 0;

    // Free multiple descriptor sets explicitly
    virtual void FreeDescriptorSets(std::span<RHIDescriptorSet*> sets) = 0;

protected:
    RHIDescriptorPool(const RHIDescriptorPoolCreateInfo& info) : RHIResource(RHIResourceType::DescriptorPool) {}
};

// =================================================================================================
// Descriptor Set
// =================================================================================================

// Descriptor write helpers
export struct RHIDescriptorBufferInfo {
    const RHIBuffer* pBuffer = nullptr;
    uint64 Offset = 0;
    uint64 Range = ~0ULL; // VK_WHOLE_SIZE
};

export struct RHIDescriptorImageInfo {
    const RHISampler* pSampler = nullptr;
    const RHITextureView* pTextureView = nullptr;
    RHILayout ImageLayout = RHILayout::ShaderReadOnly;
};

export struct RHIWriteDescriptorSet {
    RHIDescriptorSet* pDstSet = nullptr;
    uint32 DstBinding = 0;
    uint32 DstArrayElement = 0;
    uint32 DescriptorCount = 1;
    RHIDescriptorType DescriptorType = RHIDescriptorType::UniformBuffer;

    // One of these should be set based on DescriptorType
    const RHIDescriptorBufferInfo* pBufferInfos = nullptr;
    const RHIDescriptorImageInfo* pImageInfos = nullptr;
};

export struct RHICopyDescriptorSet {
    const RHIDescriptorSet* pSrcSet = nullptr;
    uint32 SrcBinding = 0;
    uint32 SrcArrayElement = 0;
    RHIDescriptorSet* pDstSet = nullptr;
    uint32 DstBinding = 0;
    uint32 DstArrayElement = 0;
    uint32 DescriptorCount = 1;
};

export class IGE_API RHIDescriptorSet : public RHIResource {
public:
    ~RHIDescriptorSet() override = default;

protected:
    RHIDescriptorSet() : RHIResource(RHIResourceType::DescriptorSet) {}
};

} // namespace iGe
