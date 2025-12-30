module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <dxgi1_6.h>

module iGe.RHI;
import :DirectX12RenderPass;
import :DirectX12Helper;

namespace iGe
{

DirectX12RenderPass::DirectX12RenderPass(const RHIRenderPassCreateInfo& info) : RHIRenderPass(info) {
    // Copy attachments
    if (!info.Attachments.empty()) { m_Attachments.assign(info.Attachments.begin(), info.Attachments.end()); }

    // Pre-convert formats to DXGI for quick access
    m_DXGIFormats.reserve(m_Attachments.size());
    for (const auto& attachment: m_Attachments) { m_DXGIFormats.push_back(RHIFormatToDXGIFormat(attachment.Format)); }

    // Copy subpasses with owned data
    if (!info.Subpasses.empty()) {
        m_Subpasses.reserve(info.Subpasses.size());
        for (const auto& srcSubpass: info.Subpasses) {
            DirectX12SubpassStorage storage;

            // Copy input attachments
            if (!srcSubpass.InputAttachments.empty()) {
                storage.InputAttachments.assign(srcSubpass.InputAttachments.begin(), srcSubpass.InputAttachments.end());
            }

            // Copy color attachments
            if (!srcSubpass.ColorAttachments.empty()) {
                storage.ColorAttachments.assign(srcSubpass.ColorAttachments.begin(), srcSubpass.ColorAttachments.end());
            }

            // Copy resolve attachments
            if (!srcSubpass.ResolveAttachments.empty()) {
                storage.ResolveAttachments.assign(srcSubpass.ResolveAttachments.begin(),
                                                  srcSubpass.ResolveAttachments.end());
            }

            // Copy depth stencil attachment index
            storage.DepthStencilAttachment = srcSubpass.DepthStencilAttachment;

            // Copy preserve attachments
            if (!srcSubpass.PreserveAttachments.empty()) {
                storage.PreserveAttachments.assign(srcSubpass.PreserveAttachments.begin(),
                                                   srcSubpass.PreserveAttachments.end());
            }

            m_Subpasses.push_back(std::move(storage));
        }
    }

    // Copy dependencies
    if (!info.Dependencies.empty()) { m_Dependencies.assign(info.Dependencies.begin(), info.Dependencies.end()); }
}

std::vector<DXGI_FORMAT> DirectX12RenderPass::GetRTVFormats(uint32 subpassIndex) const {
    std::vector<DXGI_FORMAT> formats;

    if (subpassIndex >= m_Subpasses.size()) { return formats; }

    const auto& subpass = m_Subpasses[subpassIndex];
    formats.reserve(subpass.ColorAttachments.size());

    for (uint32 attachmentIndex: subpass.ColorAttachments) {
        if (attachmentIndex != ~0u && attachmentIndex < m_DXGIFormats.size()) {
            formats.push_back(m_DXGIFormats[attachmentIndex]);
        } else {
            formats.push_back(DXGI_FORMAT_UNKNOWN);
        }
    }

    return formats;
}

DXGI_FORMAT DirectX12RenderPass::GetDSVFormat(uint32 subpassIndex) const {
    if (subpassIndex >= m_Subpasses.size()) { return DXGI_FORMAT_UNKNOWN; }

    const auto& subpass = m_Subpasses[subpassIndex];

    if (subpass.DepthStencilAttachment != ~0u && subpass.DepthStencilAttachment < m_DXGIFormats.size()) {
        return m_DXGIFormats[subpass.DepthStencilAttachment];
    }

    return DXGI_FORMAT_UNKNOWN;
}

uint32 DirectX12RenderPass::GetSampleCount(uint32 subpassIndex) const {
    if (subpassIndex >= m_Subpasses.size()) { return 1; }

    const auto& subpass = m_Subpasses[subpassIndex];

    // Try to get sample count from color attachments first
    if (!subpass.ColorAttachments.empty()) {
        uint32 attachmentIndex = subpass.ColorAttachments[0];
        if (attachmentIndex != ~0u && attachmentIndex < m_Attachments.size()) {
            return m_Attachments[attachmentIndex].SampleCount;
        }
    }

    // Fall back to depth attachment
    if (subpass.DepthStencilAttachment != ~0u && subpass.DepthStencilAttachment < m_Attachments.size()) {
        return m_Attachments[subpass.DepthStencilAttachment].SampleCount;
    }

    return 1;
}

} // namespace iGe
#endif
