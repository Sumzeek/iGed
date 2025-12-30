module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <dxgi1_6.h>

module iGe.RHI;
import :DirectX12Framebuffer;
import :DirectX12DescriptorHeap;
import :DirectX12Helper;

namespace iGe
{

DirectX12Framebuffer::DirectX12Framebuffer(ID3D12Device* device, const RHIFramebufferCreateInfo& info)
    : RHIFramebuffer(info) {
    // Store render pass reference
    auto renderPass = static_cast<const DirectX12RenderPass*>(info.pRenderPass);

    // Copy attachment view pointers
    if (!info.Attachments.empty()) { m_AttachmentViews.assign(info.Attachments.begin(), info.Attachments.end()); }

    // Create descriptor views
    {
        if (!device || m_AttachmentViews.empty()) { return; }

        if (!renderPass) { return; }

        // Process each attachment using the texture views directly
        // The DirectX12TextureView already has pre-created RTV/DSV handles
        for (size_t i = 0; i < m_AttachmentViews.size(); ++i) {
            auto* textureView = static_cast<const DirectX12TextureView*>(m_AttachmentViews[i]);
            if (!textureView) { continue; }

            // Check if this is a depth/stencil attachment
            auto format = renderPass->GetAttachment(static_cast<uint32>(i)).Format;
            if (IsDepthStencilFormat(format)) {
                // Use the DSV handle from the texture view
                m_DSVHandle = textureView->GetDSVCpu();
                m_HasDepthStencil = true;
            } else {
                // Use the RTV handle from the texture view
                m_RTVHandles.push_back(textureView->GetRTVCpu());
            }
        }
    }
}

DirectX12Framebuffer::~DirectX12Framebuffer() {
    // Note: In a complete implementation, we would free the descriptor heap allocations here
    // For now, descriptors are managed by the textures themselves or will be cleaned up
    // when the device is destroyed
}

} // namespace iGe
#endif
