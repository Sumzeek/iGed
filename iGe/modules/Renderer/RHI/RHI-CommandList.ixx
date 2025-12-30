module;
#include "iGeMacro.h"

export module iGe.RHI:RHICommandList;
import :RHIDescriptor; // MUST be first - provides complete types for RHIDescriptorSet and RHIPipelineLayout
import :RHITexture;
import :RHIRenderPass;
import :RHIComputePipeline;
import :RHIGraphicsPipeline;
import :RHIBuffer;
import :RHISampler;
import :RHIBarrier;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// Command List
// =================================================================================================

export class IGE_API RHICommandList {
public:
    virtual ~RHICommandList() = default;

    // ==========================================================================
    // Command Buffer Lifecycle
    // ==========================================================================

    virtual void Reset() = 0;
    virtual void Begin() = 0;
    virtual void End() = 0;

    // ==========================================================================
    // Render Pass Commands
    // ==========================================================================

    virtual void BeginRenderPass(const RHIRenderPassBeginInfo& info) = 0;
    virtual void EndRenderPass() = 0;
    virtual void NextSubpass() = 0;

    // ==========================================================================
    // Pipeline Binding
    // ==========================================================================

    virtual void BindGraphicsPipeline(const RHIGraphicsPipeline* pipeline) = 0;
    virtual void BindComputePipeline(const RHIComputePipeline* pipeline) = 0;

    // ==========================================================================
    // Descriptor Set Binding
    // ==========================================================================

    // virtual void BindDescriptorSets(const RHIPipelineLayout& layout, uint32 firstSet,
    //                                 const std::vector<RHIDescriptorSet*>& descriptorSets,
    //                                 const std::vector<uint32>& dynamicOffsets = {}) = 0;

    // Convenience overloads
    virtual void BindDescriptorSet(const RHIPipelineLayout* layout, uint32 setIndex,
                                   const RHIDescriptorSet* descriptorSet) = 0;

    // ==========================================================================
    // Vertex/Index Buffer Binding
    // ==========================================================================

    virtual void BindVertexBuffer(const RHIVertexBuffer* buffer, uint32 binding = 0, uint64 offset = 0) = 0;
    // virtual void BindVertexBuffers(uint32 firstBinding, const std::vector<Ref<RHIVertexBuffer>>& buffers,
    //                                const std::vector<uint64>& offsets = {}) = 0;
    virtual void BindIndexBuffer(const RHIIndexBuffer* buffer, uint64 offset = 0) = 0;

    // ==========================================================================
    // Push Constants
    // ==========================================================================

    virtual void PushConstants(const RHIPipelineLayout* layout, Flags<RHIShaderStage> stageFlags, uint32 offset,
                               uint32 size, const void* data) = 0;

    template<typename T>
    void PushConstants(const RHIPipelineLayout* layout, Flags<RHIShaderStage> stageFlags, const T& data) {
        PushConstants(layout, stageFlags, 0, sizeof(T), &data);
    }

    // ==========================================================================
    // Dynamic State
    // ==========================================================================

    virtual void SetViewport(const RHIViewport& viewport) = 0;
    // virtual void SetViewports(uint32 firstViewport, const std::vector<RHIViewport>& viewports) = 0;
    virtual void SetScissor(const RHIScissor& scissor) = 0;
    // virtual void SetScissors(uint32 firstScissor, const std::vector<RHIScissor>& scissors) = 0;
    virtual void SetLineWidth(float lineWidth) = 0;
    virtual void SetDepthBias(float constantFactor, float clamp, float slopeFactor) = 0;
    virtual void SetBlendConstants(const float blendConstants[4]) = 0;
    virtual void SetDepthBounds(float minDepthBounds, float maxDepthBounds) = 0;
    virtual void SetStencilCompareMask(bool front, bool back, uint32 compareMask) = 0;
    virtual void SetStencilWriteMask(bool front, bool back, uint32 writeMask) = 0;
    virtual void SetStencilReference(bool front, bool back, uint32 reference) = 0;

    // ==========================================================================
    // Draw Commands
    // ==========================================================================

    virtual void Draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 0,
                      uint32 firstInstance = 0) = 0;
    virtual void DrawIndexed(uint32 indexCount, uint32 instanceCount = 1, uint32 firstIndex = 0, int32 vertexOffset = 0,
                             uint32 firstInstance = 0) = 0;

    // ==========================================================================
    // Compute Commands
    // ==========================================================================

    virtual void Dispatch(uint32 groupCountX, uint32 groupCountY = 1, uint32 groupCountZ = 1) = 0;

    // ==========================================================================
    // Resource Barriers/Transitions
    // ==========================================================================

    virtual void ResourceBarrier(const RHITexture* texture, RHILayout oldLayout, RHILayout newLayout) = 0;
    virtual void PipelineBarrier(const RHIBarrierBatch* barriers) = 0;

    // // Convenience methods for texture layout transitions
    // virtual void TransitionTextureLayout(const RHITexture& texture, RHILayout oldLayout, RHILayout newLayout,
    //                                      const RHITextureSubresourceRange& range = {}) = 0;

    // ==========================================================================
    // Copy Commands
    // ==========================================================================

    virtual void CopyBufferToTexture(const RHIBuffer* srcBuffer, const RHITexture* dstTexture) = 0;
    virtual void CopyTextureToBuffer(const RHITexture* srcTexture, const RHIBuffer* dstBuffer) = 0;
    virtual void CopyBuffer(const RHIBuffer* srcBuffer, const RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset,
                            uint64 size) = 0;
    // virtual void CopyTexture(const RHITexture& srcTexture, const RHITexture& dstTexture,
    //                          const RHITextureSubresourceLayers& srcSubresource = {},
    //                          const RHITextureSubresourceLayers& dstSubresource = {}) = 0;
    virtual void BlitTexture(const RHITexture* srcTexture, const RHITexture* dstTexture,
                             RHISamplerFilter filter = RHISamplerFilter::Linear) = 0;

    // ==========================================================================
    // Clear Commands
    // ==========================================================================

    virtual void ClearColorAttachment(uint32 attachmentIndex, const float color[4], const RHIRect2D& rect) = 0;
    virtual void ClearDepthStencilAttachment(float depth, uint32 stencil, bool clearDepth, bool clearStencil,
                                             const RHIRect2D& rect) = 0;
    virtual void ClearTexture(const RHITexture* texture, const float color[4]) = 0;
    virtual void ClearBuffer(const RHIBuffer* buffer, uint32 value, uint64 offset = 0, uint64 size = ~0ULL) = 0;

    // ==========================================================================
    // Debug Commands
    // ==========================================================================

    virtual void BeginDebugLabel(const std::string& label, const float color[4] = nullptr) = 0;
    virtual void EndDebugLabel() = 0;
    virtual void InsertDebugLabel(const std::string& label, const float color[4] = nullptr) = 0;

protected:
    RHICommandList() {}
};

// // =================================================================================================
// // Immediate Command List (for single-shot commands)
// // =================================================================================================
//
// export class IGE_API RHICommandListImmediate {
// public:
//     virtual ~RHICommandListImmediate() = default;
//
//     virtual void Flush() = 0;
//     virtual void GenerateMips(Ref<RHITexture> src) = 0;
//
//     // Immediate texture upload
//     virtual void UploadTexture(Ref<RHITexture> texture, const void* data, uint64 dataSize,
//                                const RHITextureSubresourceLayers& subresource = {}) = 0;
//
//     // Immediate buffer upload
//     virtual void UploadBuffer(Ref<RHIBuffer> buffer, const void* data, uint64 dataSize, uint64 offset = 0) = 0;
//
// protected:
//     RHICommandListImmediate() {}
// };

} // namespace iGe
