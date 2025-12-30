module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12CommandList;
import :DirectX12CommandPool;
import :DirectX12Buffer;
import :DirectX12Texture;
import :DirectX12TextureView;
import :DirectX12GraphicsPipeline;
import :DirectX12ComputePipeline;
import :DirectX12Descriptor;
import :DirectX12RenderPass;
import :DirectX12Helper;

namespace iGe
{

D3D12_RESOURCE_STATES RHILayoutToD3D12State(RHILayout layout) {
    switch (layout) {
        case RHILayout::Undefined:
            return D3D12_RESOURCE_STATE_COMMON;
        case RHILayout::General:
            return D3D12_RESOURCE_STATE_COMMON;
        case RHILayout::ColorAttachment:
            return D3D12_RESOURCE_STATE_RENDER_TARGET;
        case RHILayout::DepthStencilAttachment:
            return D3D12_RESOURCE_STATE_DEPTH_WRITE;
        case RHILayout::DepthStencilReadOnly:
            return D3D12_RESOURCE_STATE_DEPTH_READ;
        case RHILayout::ShaderReadOnly:
            return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        case RHILayout::TransferSrc:
            return D3D12_RESOURCE_STATE_COPY_SOURCE;
        case RHILayout::TransferDst:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        case RHILayout::Present:
            return D3D12_RESOURCE_STATE_PRESENT;
        default:
            return D3D12_RESOURCE_STATE_COMMON;
    }
}

// =================================================================================================
// DirectX12CommandList
// =================================================================================================

DirectX12CommandList::DirectX12CommandList(ID3D12Device* device, RHICommandPool* pool) : m_Device(device) {
    m_Pool = static_cast<DirectX12CommandPool*>(pool);

    // Create command list in closed state
    HRESULT hr = m_Device->CreateCommandList(0, m_Pool->GetCommandListType(), m_Pool->GetNativeAllocator(), nullptr,
                                             IID_PPV_ARGS(&m_CommandList));

    if (FAILED(hr)) {
        Internal::LogError("Failed to create command list");
        return;
    }

    // Close it immediately since we'll reset it when Begin() is called
    m_CommandList->Close();

    // Try to get CommandList4 for ray tracing support
    m_CommandList.As(&m_CommandList4);
}

DirectX12CommandList::~DirectX12CommandList() = default;

void DirectX12CommandList::Reset() {
    m_IsRecording = false;
    m_PendingBarriers.clear();
    m_InRenderPass = false;
    m_CurrentRTVs.clear();
    m_HasDSV = false;
    m_IsComputePipeline = false;

    HRESULT hr = m_CommandList->Reset(m_Pool->GetNativeAllocator(), nullptr);
    if (FAILED(hr)) {
        Internal::LogError("Failed to reset command list for recording");
        return;
    }
}

void DirectX12CommandList::Begin() { m_IsRecording = true; }

void DirectX12CommandList::End() {
    // Flush any pending barriers
    FlushResourceBarriers();

    HRESULT hr = m_CommandList->Close();
    if (FAILED(hr)) { Internal::LogError("Failed to close command list"); }
    m_IsRecording = false;
}

void DirectX12CommandList::BeginRenderPass(const RHIRenderPassBeginInfo& beginInfo) {
    m_InRenderPass = true;
    m_CurrentRTVs.clear();
    m_HasDSV = false;

    // Get render pass info for LoadOp/StoreOp
    const auto* renderPass = static_cast<const DirectX12RenderPass*>(beginInfo.pRenderPass);

    // Get render target views from color attachments
    for (size_t i = 0; i < beginInfo.ColorAttachments.size(); ++i) {
        const auto& binding = beginInfo.ColorAttachments[i];
        if (!binding.pTextureView) continue;

        auto* dxTextureView = static_cast<const DirectX12TextureView*>(binding.pTextureView);
        D3D12_CPU_DESCRIPTOR_HANDLE rtv = dxTextureView->GetRTVCpu();
        m_CurrentRTVs.push_back(rtv);

        // Clear if specified in the render pass attachment description
        if (i < renderPass->GetAttachmentCount() &&
            renderPass->GetAttachment(static_cast<uint32>(i)).LoadOp == RHILoadOp::Clear) {
            m_CommandList->ClearRenderTargetView(rtv, binding.ClearValue.Color, 0, nullptr);
        }
    }

    // Handle depth stencil attachment
    if (beginInfo.pDepthStencilAttachment && beginInfo.pDepthStencilAttachment->pTextureView) {
        auto* dxDepthView = static_cast<const DirectX12TextureView*>(beginInfo.pDepthStencilAttachment->pTextureView);
        m_CurrentDSV = dxDepthView->GetDSVCpu();
        m_HasDSV = true;

        // Find depth attachment index in render pass
        uint32 depthAttachmentIndex = static_cast<uint32>(beginInfo.ColorAttachments.size());

        // Clear depth/stencil if specified
        D3D12_CLEAR_FLAGS clearFlags = {};
        bool shouldClear = false;

        if (depthAttachmentIndex < renderPass->GetAttachmentCount()) {
            const auto& depthDesc = renderPass->GetAttachment(depthAttachmentIndex);
            if (depthDesc.LoadOp == RHILoadOp::Clear) {
                clearFlags |= D3D12_CLEAR_FLAG_DEPTH;
                shouldClear = true;
            }
            if (depthDesc.StencilLoadOp == RHILoadOp::Clear) {
                clearFlags |= D3D12_CLEAR_FLAG_STENCIL;
                shouldClear = true;
            }
        }

        if (shouldClear) {
            float depthValue = beginInfo.pDepthStencilAttachment->ClearValue.DepthStencil.Depth;
            UINT8 stencilValue = static_cast<UINT8>(beginInfo.pDepthStencilAttachment->ClearValue.DepthStencil.Stencil);
            m_CommandList->ClearDepthStencilView(m_CurrentDSV, clearFlags, depthValue, stencilValue, 0, nullptr);
        }
    }

    // Bind render targets
    m_CommandList->OMSetRenderTargets(static_cast<UINT>(m_CurrentRTVs.size()),
                                      m_CurrentRTVs.empty() ? nullptr : m_CurrentRTVs.data(), FALSE,
                                      m_HasDSV ? &m_CurrentDSV : nullptr);

    // Set viewport and scissor from render area
    D3D12_VIEWPORT viewport = {};
    viewport.TopLeftX = static_cast<float>(beginInfo.RenderAreaOffset.X);
    viewport.TopLeftY = static_cast<float>(beginInfo.RenderAreaOffset.Y);
    viewport.Width = static_cast<float>(beginInfo.RenderAreaExtent.Width);
    viewport.Height = static_cast<float>(beginInfo.RenderAreaExtent.Height);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    m_CommandList->RSSetViewports(1, &viewport);

    D3D12_RECT scissor = {};
    scissor.left = beginInfo.RenderAreaOffset.X;
    scissor.top = beginInfo.RenderAreaOffset.Y;
    scissor.right = beginInfo.RenderAreaOffset.X + static_cast<LONG>(beginInfo.RenderAreaExtent.Width);
    scissor.bottom = beginInfo.RenderAreaOffset.Y + static_cast<LONG>(beginInfo.RenderAreaExtent.Height);
    m_CommandList->RSSetScissorRects(1, &scissor);
}

void DirectX12CommandList::EndRenderPass() {
    m_InRenderPass = false;
    m_CurrentRTVs.clear();
    m_HasDSV = false;
}

void DirectX12CommandList::NextSubpass() {
    // D3D12 doesn't have native subpasses
}

void DirectX12CommandList::BindGraphicsPipeline(const RHIGraphicsPipeline* pipeline) {
    auto dxPipeline = static_cast<const DirectX12GraphicsPipeline*>(pipeline);

    m_CommandList->SetPipelineState(dxPipeline->GetNativePSO());
    m_CommandList->SetGraphicsRootSignature(dxPipeline->GetRootSignature());
    m_CommandList->IASetPrimitiveTopology(dxPipeline->GetPrimitiveTopology());
    m_IsComputePipeline = false;
}

void DirectX12CommandList::BindComputePipeline(const RHIComputePipeline* pipeline) {
    auto dxPipeline = static_cast<const DirectX12ComputePipeline*>(pipeline);

    m_CommandList->SetPipelineState(dxPipeline->GetNativePSO());
    m_CommandList->SetComputeRootSignature(dxPipeline->GetRootSignature());
    m_IsComputePipeline = true;
}

void DirectX12CommandList::BindDescriptorSet(const RHIPipelineLayout* layout, uint32 setIndex,
                                             const RHIDescriptorSet* descriptorSet) {
    auto dxLayout = static_cast<const DirectX12PipelineLayout*>(layout);
    auto dxSet = static_cast<const DirectX12DescriptorSet*>(descriptorSet);
    if (!dxSet) { return; }

    // Set descriptor heaps
    if (dxSet->GetPool()) {
        ID3D12DescriptorHeap* heaps[2] = {};
        uint32 heapCount = 0;

        if (dxSet->HasCBVSRVUAV()) { heaps[heapCount++] = dxSet->GetPool()->GetCBVSRVUAVHeap(); }
        if (dxSet->HasSamplers()) { heaps[heapCount++] = dxSet->GetPool()->GetSamplerHeap(); }

        if (heapCount > 0) { m_CommandList->SetDescriptorHeaps(heapCount, heaps); }
    }

    // Try to get root parameter index from layout, fallback to default indices
    int32 cbvSrvUavRootIndex = -1;
    int32 samplerRootIndex = -1;

    if (dxLayout) {
        cbvSrvUavRootIndex = dxLayout->GetRootParameterIndex(setIndex, 0);
        samplerRootIndex = dxLayout->GetRootParameterIndex(setIndex, 1);
    }

    // Fallback: use default root signature layout
    if (cbvSrvUavRootIndex < 0) { cbvSrvUavRootIndex = 0; }
    if (samplerRootIndex < 0) { samplerRootIndex = 1; }

    // Bind CBV/SRV/UAV table if present
    if (dxSet->HasCBVSRVUAV()) {
        D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle = dxSet->GetCBVSRVUAVTableGPUHandle();
        if (m_IsComputePipeline) {
            m_CommandList->SetComputeRootDescriptorTable(cbvSrvUavRootIndex, gpuHandle);
        } else {
            m_CommandList->SetGraphicsRootDescriptorTable(cbvSrvUavRootIndex, gpuHandle);
        }
    }

    // Bind Sampler table if present
    if (dxSet->HasSamplers()) {
        D3D12_GPU_DESCRIPTOR_HANDLE samplerHandle = dxSet->GetSamplerTableGPUHandle();
        if (m_IsComputePipeline) {
            m_CommandList->SetComputeRootDescriptorTable(samplerRootIndex, samplerHandle);
        } else {
            m_CommandList->SetGraphicsRootDescriptorTable(samplerRootIndex, samplerHandle);
        }
    }
}

void DirectX12CommandList::BindVertexBuffer(const RHIVertexBuffer* buffer, uint32 binding, uint64 offset) {
    auto dxBuffer = static_cast<const DirectX12VertexBuffer*>(buffer);
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(dxBuffer->GetNativeHandle());

    D3D12_VERTEX_BUFFER_VIEW view = {};
    view.BufferLocation = resource->GetGPUVirtualAddress() + offset;
    view.SizeInBytes = static_cast<UINT>(dxBuffer->GetSize() - offset);
    view.StrideInBytes = static_cast<UINT>(dxBuffer->GetStride());

    m_CommandList->IASetVertexBuffers(binding, 1, &view);
}

void DirectX12CommandList::BindIndexBuffer(const RHIIndexBuffer* buffer, uint64 offset) {
    auto dxBuffer = static_cast<const DirectX12IndexBuffer*>(buffer);
    ID3D12Resource* resource = static_cast<ID3D12Resource*>(dxBuffer->GetNativeHandle());

    D3D12_INDEX_BUFFER_VIEW view = {};
    view.BufferLocation = resource->GetGPUVirtualAddress() + offset;
    view.SizeInBytes = static_cast<UINT>(dxBuffer->GetSize() - offset);
    view.Format = (buffer->GetFormat() == RHIIndexFormat::Uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

    m_CommandList->IASetIndexBuffer(&view);
}

void DirectX12CommandList::PushConstants(const RHIPipelineLayout* layout, Flags<RHIShaderStage> stageFlags,
                                         uint32 offset, uint32 size, const void* data) {
    auto dxLayout = static_cast<const DirectX12PipelineLayout*>(layout);

    int32 rootParamIndex = dxLayout->GetPushConstantRootIndex();
    if (rootParamIndex >= 0) {
        uint32 num32BitValues = size / 4;
        uint32 destOffset = offset / 4;

        // Use appropriate method based on current pipeline type
        if (m_IsComputePipeline) {
            m_CommandList->SetComputeRoot32BitConstants(rootParamIndex, num32BitValues, data, destOffset);
        } else {
            m_CommandList->SetGraphicsRoot32BitConstants(rootParamIndex, num32BitValues, data, destOffset);
        }
    }
}

void DirectX12CommandList::SetViewport(const RHIViewport& viewport) {
    D3D12_VIEWPORT vp = {};
    vp.TopLeftX = viewport.X;
    vp.TopLeftY = viewport.Y;
    vp.Width = viewport.Width;
    vp.Height = viewport.Height;
    vp.MinDepth = viewport.MinDepth;
    vp.MaxDepth = viewport.MaxDepth;
    m_CommandList->RSSetViewports(1, &vp);
}

void DirectX12CommandList::SetScissor(const RHIScissor& scissor) {
    D3D12_RECT rect = {};
    rect.left = scissor.X;
    rect.top = scissor.Y;
    rect.right = scissor.X + static_cast<LONG>(scissor.Width);
    rect.bottom = scissor.Y + static_cast<LONG>(scissor.Height);
    m_CommandList->RSSetScissorRects(1, &rect);
}

void DirectX12CommandList::SetLineWidth(float lineWidth) {
    // D3D12 doesn't support dynamic line width
}

void DirectX12CommandList::SetDepthBias(float constantFactor, float clamp, float slopeFactor) {
    // D3D12 depth bias is set in the pipeline state
}

void DirectX12CommandList::SetBlendConstants(const float blendConstants[4]) {
    m_CommandList->OMSetBlendFactor(blendConstants);
}

void DirectX12CommandList::SetDepthBounds(float minDepthBounds, float maxDepthBounds) {
    Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList1> cmdList1;
    if (SUCCEEDED(m_CommandList.As(&cmdList1))) { cmdList1->OMSetDepthBounds(minDepthBounds, maxDepthBounds); }
}

void DirectX12CommandList::SetStencilCompareMask(bool front, bool back, uint32 compareMask) {
    // D3D12 stencil masks are set in the pipeline state
}

void DirectX12CommandList::SetStencilWriteMask(bool front, bool back, uint32 writeMask) {
    // D3D12 stencil masks are set in the pipeline state
}

void DirectX12CommandList::SetStencilReference(bool front, bool back, uint32 reference) {
    m_CommandList->OMSetStencilRef(reference);
}

void DirectX12CommandList::Draw(uint32 vertexCount, uint32 instanceCount, uint32 firstVertex, uint32 firstInstance) {
    m_CommandList->DrawInstanced(vertexCount, instanceCount, firstVertex, firstInstance);
}

void DirectX12CommandList::DrawIndexed(uint32 indexCount, uint32 instanceCount, uint32 firstIndex, int32 vertexOffset,
                                       uint32 firstInstance) {
    m_CommandList->DrawIndexedInstanced(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

void DirectX12CommandList::Dispatch(uint32 groupCountX, uint32 groupCountY, uint32 groupCountZ) {
    m_CommandList->Dispatch(groupCountX, groupCountY, groupCountZ);
}

void DirectX12CommandList::ResourceBarrier(const RHITexture* texture, RHILayout oldLayout, RHILayout newLayout) {
    if (oldLayout == newLayout) { return; }

    auto resource = static_cast<ID3D12Resource*>(texture->GetNativeHandle());
    if (!resource) { return; }

    D3D12_RESOURCE_STATES stateBefore = RHILayoutToD3D12State(oldLayout);
    D3D12_RESOURCE_STATES stateAfter = RHILayoutToD3D12State(newLayout);

    // D3D12 requires before and after states to be different
    // Note: D3D12_RESOURCE_STATE_PRESENT == D3D12_RESOURCE_STATE_COMMON (both are 0)
    if (stateBefore == stateAfter) { return; }

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = resource;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = stateBefore;
    barrier.Transition.StateAfter = stateAfter;

    m_PendingBarriers.push_back(barrier);
    FlushResourceBarriers();
}

void DirectX12CommandList::ResourceBarrier(const RHIBuffer* buffer, RHILayout oldLayout, RHILayout newLayout) {
    auto resource = static_cast<ID3D12Resource*>(buffer->GetNativeHandle());

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barrier.UAV.pResource = resource;

    m_PendingBarriers.push_back(barrier);
    FlushResourceBarriers();
}

void DirectX12CommandList::PipelineBarrier(const RHIBarrierBatch* barriers) {
    // Process texture barriers
    for (const auto& texBarrier: barriers->TextureBarriers) {
        if (texBarrier.pTexture) { ResourceBarrier(texBarrier.pTexture, texBarrier.OldLayout, texBarrier.NewLayout); }
    }
    // Process buffer barriers
    for (const auto& bufBarrier: barriers->BufferBarriers) {
        if (bufBarrier.pBuffer) { ResourceBarrier(bufBarrier.pBuffer, RHILayout::General, RHILayout::General); }
    }
}

void DirectX12CommandList::FlushResourceBarriers() {
    if (!m_PendingBarriers.empty()) {
        m_CommandList->ResourceBarrier(static_cast<UINT>(m_PendingBarriers.size()), m_PendingBarriers.data());
        m_PendingBarriers.clear();
    }
}

void DirectX12CommandList::CopyBufferToTexture(const RHIBuffer* srcBuffer, const RHITexture* dstTexture) {
    auto srcResource = static_cast<ID3D12Resource*>(srcBuffer->GetNativeHandle());
    auto dstResource = static_cast<ID3D12Resource*>(dstTexture->GetNativeHandle());

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = dstResource;
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    dstLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = srcResource;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    D3D12_RESOURCE_DESC textureDesc = dstResource->GetDesc();
    UINT64 totalBytes = 0;
    m_Device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &srcLocation.PlacedFootprint, nullptr, nullptr, &totalBytes);

    m_CommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
}

void DirectX12CommandList::CopyTextureToBuffer(const RHITexture* srcTexture, const RHIBuffer* dstBuffer) {
    auto srcResource = static_cast<ID3D12Resource*>(srcTexture->GetNativeHandle());
    auto dstResource = static_cast<ID3D12Resource*>(dstBuffer->GetNativeHandle());

    D3D12_TEXTURE_COPY_LOCATION srcLocation = {};
    srcLocation.pResource = srcResource;
    srcLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
    srcLocation.SubresourceIndex = 0;

    D3D12_TEXTURE_COPY_LOCATION dstLocation = {};
    dstLocation.pResource = dstResource;
    dstLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;

    D3D12_RESOURCE_DESC textureDesc = srcResource->GetDesc();
    UINT64 totalBytes = 0;
    m_Device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &dstLocation.PlacedFootprint, nullptr, nullptr, &totalBytes);

    m_CommandList->CopyTextureRegion(&dstLocation, 0, 0, 0, &srcLocation, nullptr);
}

void DirectX12CommandList::CopyBuffer(const RHIBuffer* srcBuffer, const RHIBuffer* dstBuffer, uint64 srcOffset,
                                      uint64 dstOffset, uint64 size) {
    auto srcResource = static_cast<ID3D12Resource*>(srcBuffer->GetNativeHandle());
    auto dstResource = static_cast<ID3D12Resource*>(dstBuffer->GetNativeHandle());

    m_CommandList->CopyBufferRegion(dstResource, dstOffset, srcResource, srcOffset, size);
}

void DirectX12CommandList::BlitTexture(const RHITexture* srcTexture, const RHITexture* dstTexture,
                                       RHISamplerFilter filter) {
    Internal::LogWarn("BlitTexture not implemented - requires compute shader");
}

void DirectX12CommandList::ClearColorAttachment(uint32 attachmentIndex, const float color[4], const RHIRect2D& rect) {
    if (attachmentIndex < m_CurrentRTVs.size()) {
        auto offset = rect.Offset;
        auto extent = rect.Extent;
        D3D12_RECT d3dRect = {offset.X, offset.Y, static_cast<LONG>(offset.X + extent.Width),
                              static_cast<LONG>(offset.Y + extent.Height)};
        m_CommandList->ClearRenderTargetView(m_CurrentRTVs[attachmentIndex], color, 1, &d3dRect);
    }
}

void DirectX12CommandList::ClearDepthStencilAttachment(float depth, uint32 stencil, bool clearDepth, bool clearStencil,
                                                       const RHIRect2D& rect) {
    if (!m_HasDSV) { return; }

    D3D12_CLEAR_FLAGS flags = {};
    if (clearDepth) flags |= D3D12_CLEAR_FLAG_DEPTH;
    if (clearStencil) flags |= D3D12_CLEAR_FLAG_STENCIL;

    auto offset = rect.Offset;
    auto extent = rect.Extent;
    D3D12_RECT d3dRect = {offset.X, offset.Y, static_cast<LONG>(offset.X + extent.Width),
                          static_cast<LONG>(offset.Y + extent.Height)};
    m_CommandList->ClearDepthStencilView(m_CurrentDSV, flags, depth, static_cast<UINT8>(stencil), 1, &d3dRect);
}

void DirectX12CommandList::ClearTexture(const RHITexture* texture, const float color[4]) {
    auto* dxTexture = static_cast<const DirectX12Texture*>(texture);
    if (dxTexture) { m_CommandList->ClearRenderTargetView(dxTexture->GetRTV(), color, 0, nullptr); }
}

void DirectX12CommandList::ClearBuffer(const RHIBuffer* buffer, uint32 value, uint64 offset, uint64 size) {
    // Would need UAV clear
    Internal::LogWarn("ClearBuffer not implemented - requires UAV");
}

void DirectX12CommandList::BeginDebugLabel(const std::string& label, const float color[4]) {
    #if defined(USE_PIX)
    if (color) {
        PIXBeginEvent(m_CommandList.Get(),
                      PIX_COLOR(static_cast<BYTE>(color[0] * 255), static_cast<BYTE>(color[1] * 255),
                                static_cast<BYTE>(color[2] * 255)),
                      label.c_str());
    } else {
        PIXBeginEvent(m_CommandList.Get(), PIX_COLOR_DEFAULT, label.c_str());
    }
    #endif
}

void DirectX12CommandList::EndDebugLabel() {
    #if defined(USE_PIX)
    PIXEndEvent(m_CommandList.Get());
    #endif
}

void DirectX12CommandList::InsertDebugLabel(const std::string& label, const float color[4]) {
    #if defined(USE_PIX)
    if (color) {
        PIXSetMarker(m_CommandList.Get(),
                     PIX_COLOR(static_cast<BYTE>(color[0] * 255), static_cast<BYTE>(color[1] * 255),
                               static_cast<BYTE>(color[2] * 255)),
                     label.c_str());
    } else {
        PIXSetMarker(m_CommandList.Get(), PIX_COLOR_DEFAULT, label.c_str());
    }
    #endif
}

} // namespace iGe
#endif
