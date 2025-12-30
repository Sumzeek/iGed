module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12Semaphore;
import :RHISemaphore;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// DirectX12Semaphore
// =================================================================================================

// Note: D3D12 doesn't have explicit semaphores like Vulkan.
// GPU-GPU synchronization is handled via fences and command queue ordering.
// This class provides a compatible interface for the RHI abstraction.

export class IGE_API DirectX12Semaphore : public RHISemaphore {
public:
    DirectX12Semaphore(ID3D12Device* device);
    ~DirectX12Semaphore() override = default;

    // For timeline semaphore behavior
    ID3D12Fence* GetFence() const { return m_Fence.Get(); }

    // Signal from GPU
    uint64 GetNextSignalValue() { return ++m_SignalValue; }
    uint64 GetLastSignaledValue() const { return m_SignalValue; }

private:
    Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
    uint64 m_SignalValue = 0;
};

} // namespace iGe
#endif
