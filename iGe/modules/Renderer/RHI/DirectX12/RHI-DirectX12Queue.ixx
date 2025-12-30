module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12Queue;
import :RHIQueue;
import :DirectX12Fence;
import :DirectX12Semaphore;
import :DirectX12CommandList;

namespace iGe
{

// =================================================================================================
// DirectX12Queue
// =================================================================================================

export class IGE_API DirectX12Queue : public RHIQueue {
public:
    DirectX12Queue(ID3D12Device* device, RHIQueueType type, uint32 queueIndex = 0);
    DirectX12Queue(const RHIQueueCreateInfo& info, Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue);
    ~DirectX12Queue() override;

    // Implement base class virtual method
    void Submit(const RHICommandList* commandList, RHIFence* fence = nullptr,
                std::span<RHISemaphore*> waitSemaphores = {}, std::span<RHISemaphore*> signalSemaphores = {}) override;

    // Submit multiple command lists
    void SubmitCommandLists(std::span<const RHICommandList*> commandLists, RHIFence* signalFence = nullptr);

    // Wait for all submitted work to complete
    void WaitIdle() override;

    // Signal a fence from GPU
    void Signal(DirectX12Fence* fence, uint64 value);
    void Signal(DirectX12Semaphore* semaphore);

    // Wait on GPU for a fence value
    void Wait(DirectX12Fence* fence, uint64 value);
    void Wait(DirectX12Semaphore* semaphore, uint64 value);

    // Getters
    RHIQueueType GetQueueType() const { return m_QueueType; }
    uint32 GetQueueIndex() const { return m_QueueIndex; }

    // Native handle access
    ID3D12CommandQueue* GetCommandQueue() const { return m_CommandQueue.Get(); }

    void* GetNativeHandle() const override { return m_CommandQueue.Get(); }

    // Execute command lists and return a fence value for synchronization
    uint64 ExecuteCommandLists(const std::vector<ID3D12CommandList*>& commandLists);

private:
    Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_CommandQueue;
    uint32 m_QueueIndex;

    // Internal fence for WaitIdle
    Scope<DirectX12Fence> m_InternalFence;
};

} // namespace iGe
#endif
