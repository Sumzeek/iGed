module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12ImGuiContext;
import :RHIImGuiContext;
import iGe.Common;

namespace iGe
{

export class IGE_API DirectX12ImGuiContext : public RHIImGuiContext {
public:
    DirectX12ImGuiContext();
    virtual ~DirectX12ImGuiContext() override;

    virtual void Begin(uint32 frameIndex = 0) override;
    virtual void End() override;
    virtual void SetRenderTarget(RHITexture& target) override { m_RenderTarget = &target; }

protected:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_Allocator;
    std::vector<Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>> m_CommandLists;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_SrvDescHeap = nullptr;

    uint32 m_FrameIndex = 0;
    RHITexture* m_RenderTarget = nullptr;
};

} // namespace iGe
#endif
