module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #define NOMINMAX
    #include <d3d12.h>
    #include <dxgi1_6.h>

export module iGe.RHI:DirectX12Framebuffer;
import :RHIFramebuffer;
import :DirectX12Texture;
import :DirectX12TextureView;
import :DirectX12RenderPass;

namespace iGe
{

// =================================================================================================
// DirectX12Framebuffer
// =================================================================================================

// Note: D3D12 doesn't have an explicit framebuffer object like Vulkan.
// This class manages the render target views (RTVs) and depth-stencil views (DSVs)
// that correspond to a Vulkan-style framebuffer concept.

export class IGE_API DirectX12Framebuffer : public RHIFramebuffer {
public:
    DirectX12Framebuffer(ID3D12Device* device, const RHIFramebufferCreateInfo& info);
    ~DirectX12Framebuffer() override;

    // Get RTV handles for binding
    const std::vector<D3D12_CPU_DESCRIPTOR_HANDLE>& GetRTVHandles() const { return m_RTVHandles; }

    // Get DSV handle for binding (returns nullptr handle if no depth)
    D3D12_CPU_DESCRIPTOR_HANDLE GetDSVHandle() const { return m_DSVHandle; }

    // Check if framebuffer has depth attachment
    bool HasDepthStencil() const { return m_HasDepthStencil; }

    // Get color attachment count (for OMSetRenderTargets)
    uint32 GetColorAttachmentCount() const { return static_cast<uint32>(m_RTVHandles.size()); }

    // Get attachment views
    const std::vector<const RHITextureView*>& GetAttachmentViews() const { return m_AttachmentViews; }

private:
    // Stored attachment views (non-owning pointers)
    std::vector<const RHITextureView*> m_AttachmentViews;

    std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_RTVHandles;
    D3D12_CPU_DESCRIPTOR_HANDLE m_DSVHandle = {};
    bool m_HasDepthStencil = false;

    // Descriptor heap allocations (for cleanup)
    std::vector<uint32> m_RTVHeapOffsets;
    uint32 m_DSVHeapOffset = std::numeric_limits<uint32>::max();
};

} // namespace iGe
#endif
