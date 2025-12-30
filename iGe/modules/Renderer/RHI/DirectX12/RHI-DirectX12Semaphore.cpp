module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12Semaphore;

namespace iGe
{

using Microsoft::WRL::ComPtr;

// =================================================================================================
// DirectX12Semaphore
// =================================================================================================

DirectX12Semaphore::DirectX12Semaphore(ID3D12Device* device) {
    if (!device) { Internal::LogError("DirectX12Semaphore: Device is null"); }

    // D3D12 uses fences for GPU-GPU synchronization
    // Create a fence that will be used as a timeline semaphore
    HRESULT hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));

    if (FAILED(hr)) { Internal::LogError("DirectX12Semaphore: Failed to create fence"); }
}

} // namespace iGe
#endif
