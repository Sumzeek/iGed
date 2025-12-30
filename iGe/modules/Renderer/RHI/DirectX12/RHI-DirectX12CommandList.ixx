module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12CommandList;
import :RHICommandList;
import :DirectX12CommandPool;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// DirectX12CommandList
// =================================================================================================

export class IGE_API DirectX12CommandList : public RHICommandList {
public:
    DirectX12CommandList(ID3D12Device* device, RHICommandPool* pool);
    ~DirectX12CommandList() override;

    // ==========================================================================
    // Command Buffer Lifecycle
    // ==========================================================================

    void Reset() override;
    void Begin() override;
    void End() override;

    // ==========================================================================
    // Render Pass Commands
    // ==========================================================================

    void BeginRenderPass(const RHIRenderPassBeginInfo& info) override;
    void EndRenderPass() override;
    void NextSubpass() override;

    // ==========================================================================
    // Pipeline Binding
    // ==========================================================================

    void BindGraphicsPipeline(const RHIGraphicsPipeline* pipeline) override;
    void BindComputePipeline(const RHIComputePipeline* pipeline) override;

    // ==========================================================================
    // Descriptor Set Binding
    // ==========================================================================

    void BindDescriptorSet(const RHIPipelineLayout* layout, uint32 setIndex,
                           const RHIDescriptorSet* descriptorSet) override;

    // ==========================================================================
    // Vertex/Index Buffer Binding
    // ==========================================================================

    void BindVertexBuffer(const RHIVertexBuffer* buffer, uint32 binding = 0, uint64 offset = 0) override;
    void BindIndexBuffer(const RHIIndexBuffer* buffer, uint64 offset = 0) override;

    // ==========================================================================
    // Push Constants
    // ==========================================================================

    void PushConstants(const RHIPipelineLayout* layout, Flags<RHIShaderStage> stageFlags, uint32 offset, uint32 size,
                       const void* data) override;

    // ==========================================================================
    // Dynamic State
    // ==========================================================================

    void SetViewport(const RHIViewport& viewport) override;
    void SetScissor(const RHIScissor& scissor) override;
    void SetLineWidth(float lineWidth) override;
    void SetDepthBias(float constantFactor, float clamp, float slopeFactor) override;
    void SetBlendConstants(const float blendConstants[4]) override;
    void SetDepthBounds(float minDepthBounds, float maxDepthBounds) override;
    void SetStencilCompareMask(bool front, bool back, uint32 compareMask) override;
    void SetStencilWriteMask(bool front, bool back, uint32 writeMask) override;
    void SetStencilReference(bool front, bool back, uint32 reference) override;

    // ==========================================================================
    // Draw Commands
    // ==========================================================================

    void Draw(uint32 vertexCount, uint32 instanceCount = 1, uint32 firstVertex = 0, uint32 firstInstance = 0) override;
    void DrawIndexed(uint32 indexCount, uint32 instanceCount = 1, uint32 firstIndex = 0, int32 vertexOffset = 0,
                     uint32 firstInstance = 0) override;

    // ==========================================================================
    // Compute Commands
    // ==========================================================================

    void Dispatch(uint32 groupCountX, uint32 groupCountY = 1, uint32 groupCountZ = 1) override;

    // ==========================================================================
    // Resource Barriers/Transitions
    // ==========================================================================

    void ResourceBarrier(const RHITexture* texture, RHILayout oldLayout, RHILayout newLayout) override;
    void ResourceBarrier(const RHIBuffer* buffer, RHILayout oldLayout, RHILayout newLayout);
    void PipelineBarrier(const RHIBarrierBatch* barriers) override;

    // ==========================================================================
    // Copy Commands
    // ==========================================================================

    void CopyBufferToTexture(const RHIBuffer* srcBuffer, const RHITexture* dstTexture) override;
    void CopyTextureToBuffer(const RHITexture* srcTexture, const RHIBuffer* dstBuffer) override;
    void CopyBuffer(const RHIBuffer* srcBuffer, const RHIBuffer* dstBuffer, uint64 srcOffset, uint64 dstOffset,
                    uint64 size) override;
    void BlitTexture(const RHITexture* srcTexture, const RHITexture* dstTexture,
                     RHISamplerFilter filter = RHISamplerFilter::Linear) override;

    // ==========================================================================
    // Clear Commands
    // ==========================================================================

    void ClearColorAttachment(uint32 attachmentIndex, const float color[4], const RHIRect2D& rect) override;
    void ClearDepthStencilAttachment(float depth, uint32 stencil, bool clearDepth, bool clearStencil,
                                     const RHIRect2D& rect) override;
    void ClearTexture(const RHITexture* texture, const float color[4]) override;
    void ClearBuffer(const RHIBuffer* buffer, uint32 value, uint64 offset = 0, uint64 size = ~0ULL) override;

    // ==========================================================================
    // Debug Commands
    // ==========================================================================

    void BeginDebugLabel(const std::string& label, const float color[4] = nullptr) override;
    void EndDebugLabel() override;
    void InsertDebugLabel(const std::string& label, const float color[4] = nullptr) override;

    // ==========================================================================
    // Native Access
    // ==========================================================================

    ID3D12GraphicsCommandList* GetCommandList() const { return m_CommandList.Get(); }
    ID3D12GraphicsCommandList4* GetCommandList4() const { return m_CommandList4.Get(); }
    void* GetNativeHandle() const { return m_CommandList.Get(); }

private:
    void FlushResourceBarriers();

    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_CommandList;
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList4> m_CommandList4; // For ray tracing

    ID3D12Device* m_Device;
    DirectX12CommandPool* m_Pool = nullptr;
    bool m_IsRecording = false;

    // Current state
    std::vector<D3D12_RESOURCE_BARRIER> m_PendingBarriers;
    bool m_IsComputePipeline = false; // Track current pipeline type for descriptor/push constant binding

    // Render pass state
    bool m_InRenderPass = false;
    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_CurrentRTVs;
    D3D12_CPU_DESCRIPTOR_HANDLE m_CurrentDSV = {};
    bool m_HasDSV = false;
};

} // namespace iGe
#endif
