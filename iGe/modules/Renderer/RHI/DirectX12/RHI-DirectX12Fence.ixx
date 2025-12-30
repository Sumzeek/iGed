module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #define NOMINMAX
    #include <d3d12.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12Fence;
import :RHIFence;

namespace iGe
{

// =================================================================================================
// DirectX12Fence
// =================================================================================================

export class IGE_API DirectX12Fence : public RHIFence {
public:
    DirectX12Fence(ID3D12Device* device, const RHIFenceCreateInfo& info = {});

    ~DirectX12Fence() override;

    void* GetNativeHandle() const override { return m_Fence.Get(); }

    // RHI interface - Wait for fence
    bool Wait(uint64 timeout = std::numeric_limits<uint64>::max()) override;

    // RHI interface - Reset the fence
    void Reset() override;

    // Wait for the fence to reach a specific value
    void WaitForValue(uint64 value, uint32 timeoutMs = std::numeric_limits<uint32>::max());

    // Check if signaled
    bool IsSignaled() const;

    // Signal the fence from CPU
    void Signal(uint64 value);

    // Get current completed value
    uint64 GetCompletedValue() const;

    // Get the next expected value
    uint64 GetNextValue() { return ++m_NextValue; }
    // Get current next value without incrementing
    uint64 PeekNextValue() const { return m_NextValue + 1; }

    // Native handle access
    ID3D12Fence* GetFence() const { return m_Fence.Get(); }
    HANDLE GetEvent() const { return m_FenceEvent; }

private:
    void Initialize(ID3D12Device* device, uint64 initialValue);

    Microsoft::WRL::ComPtr<ID3D12Fence> m_Fence;
    HANDLE m_FenceEvent = nullptr;
    uint64 m_NextValue = 0;
    uint64 m_SignaledValue = 0; // Value to wait for in Wait()
};

} // namespace iGe
#endif
