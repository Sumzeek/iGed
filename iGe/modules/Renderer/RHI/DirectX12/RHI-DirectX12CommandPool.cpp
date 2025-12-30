module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12CommandPool;
import :DirectX12CommandList;
import :DirectX12Queue;
import :DirectX12Helper;

namespace iGe
{

// =================================================================================================
// DirectX12CommandPool
// =================================================================================================

DirectX12CommandPool::DirectX12CommandPool(ID3D12Device* device, const RHICommandPoolCreateInfo& info)
    : RHICommandPool(info) {
    auto rhiQueueType = info.pQueue->GetQueueType();
    m_CommandListType = GetD3D12CommandListType(rhiQueueType);

    HRESULT hr = device->CreateCommandAllocator(m_CommandListType, IID_PPV_ARGS(&m_CommandAllocator));
    if (FAILED(hr)) { Internal::LogError("Failed to create command allocator"); }
}

DirectX12CommandPool::~DirectX12CommandPool() {
    // Command allocator will be released automatically by ComPtr
    // All command lists should have been freed by their Scope owners before the pool is destroyed
    m_CommandAllocator.Reset();
}

void DirectX12CommandPool::Reset() {
    // Reset the command allocator
    // Note: All command lists associated with this allocator must be in the closed state
    // before calling Reset on the allocator
    HRESULT hr = m_CommandAllocator->Reset();
    if (FAILED(hr)) { Internal::LogError("Failed to reset command allocator"); }
}

} // namespace iGe
#endif
