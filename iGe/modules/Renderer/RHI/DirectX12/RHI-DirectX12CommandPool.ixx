module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12CommandPool;
import :RHICommandPool;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// DirectX12CommandPool
// =================================================================================================

export class IGE_API DirectX12CommandPool : public RHICommandPool {
public:
    DirectX12CommandPool(ID3D12Device* device, const RHICommandPoolCreateInfo& info);
    ~DirectX12CommandPool() override;

    // RHICommandPool interface
    void Reset() override;

    // Getters
    D3D12_COMMAND_LIST_TYPE GetCommandListType() const { return m_CommandListType; }
    ID3D12CommandAllocator* GetNativeAllocator() const { return m_CommandAllocator.Get(); }
    void* GetNativeHandle() const override { return m_CommandAllocator.Get(); }

private:
    Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_CommandAllocator;
    D3D12_COMMAND_LIST_TYPE m_CommandListType;
};

} // namespace iGe
#endif
