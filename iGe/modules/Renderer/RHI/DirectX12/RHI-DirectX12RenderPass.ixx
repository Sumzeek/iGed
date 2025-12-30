module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <dxgi1_6.h>

export module iGe.RHI:DirectX12RenderPass;
import :RHIRenderPass;
import :DirectX12Helper;

namespace iGe
{

// =================================================================================================
// DirectX12RenderPass
// =================================================================================================

// Note: D3D12 doesn't have explicit render pass objects like Vulkan.
// This class stores the render pass configuration for PSO creation and
// command list render pass emulation.

// Internal storage for subpass description with owned data
struct DirectX12SubpassStorage {
    std::vector<uint32> InputAttachments;
    std::vector<uint32> ColorAttachments;
    std::vector<uint32> ResolveAttachments;
    uint32 DepthStencilAttachment = ~0u;
    std::vector<uint32> PreserveAttachments;

    bool HasDepthStencilAttachment() const { return DepthStencilAttachment != ~0u; }
};

export class IGE_API DirectX12RenderPass : public RHIRenderPass {
public:
    DirectX12RenderPass(const RHIRenderPassCreateInfo& info);
    ~DirectX12RenderPass() override = default;

    // Get RTV formats for a specific subpass
    std::vector<DXGI_FORMAT> GetRTVFormats(uint32 subpassIndex = 0) const;

    // Get DSV format for a specific subpass
    DXGI_FORMAT GetDSVFormat(uint32 subpassIndex = 0) const;

    // Get sample count for a specific subpass
    uint32 GetSampleCount(uint32 subpassIndex = 0) const;

    // Attachment info
    uint32 GetAttachmentCount() const { return static_cast<uint32>(m_Attachments.size()); }
    uint32 GetSubpassCount() const { return static_cast<uint32>(m_Subpasses.size()); }

    // Get attachment description
    const RHIAttachmentDescription& GetAttachment(uint32 index) const { return m_Attachments[index]; }
    const DirectX12SubpassStorage& GetSubpass(uint32 index) const { return m_Subpasses[index]; }

    void* GetNativeHandle() const override { return nullptr; } // D3D12 doesn't have render pass objects

private:
    // Owned copies of attachment and subpass data
    std::vector<RHIAttachmentDescription> m_Attachments;
    std::vector<DirectX12SubpassStorage> m_Subpasses;
    std::vector<RHISubpassDependency> m_Dependencies;

    // Cache DXGI formats for quick access
    std::vector<DXGI_FORMAT> m_DXGIFormats;
};

} // namespace iGe
#endif
