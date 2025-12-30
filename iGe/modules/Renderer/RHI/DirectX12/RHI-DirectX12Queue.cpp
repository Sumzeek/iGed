module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12Queue;
import :DirectX12CommandList;
import :DirectX12Fence;
import :DirectX12Semaphore;
import :DirectX12Helper;

namespace iGe
{

// =================================================================================================
// DirectX12Queue
// =================================================================================================

DirectX12Queue::DirectX12Queue(ID3D12Device* device, RHIQueueType type, uint32 queueIndex)
    : RHIQueue(RHIQueueCreateInfo{type, queueIndex}), m_QueueIndex(queueIndex) {
    if (!device) { Internal::LogError("DirectX12Queue: Device is null"); }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = GetD3D12CommandListType(type);
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_CommandQueue));
    if (FAILED(hr)) { Internal::LogError("DirectX12Queue: Failed to create command queue"); }

    // Create internal fence for WaitIdle
    m_InternalFence = CreateScope<DirectX12Fence>(device);
}

DirectX12Queue::DirectX12Queue(const RHIQueueCreateInfo& info, Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue)
    : RHIQueue(info), m_CommandQueue(queue), m_QueueIndex(info.Index) {
    // Get device from queue for creating internal fence
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    if (SUCCEEDED(queue->GetDevice(IID_PPV_ARGS(&device)))) {
        m_InternalFence = CreateScope<DirectX12Fence>(device.Get());
    }
}

DirectX12Queue::~DirectX12Queue() {
    // Wait for all pending work before destruction
    WaitIdle();
}

void DirectX12Queue::Submit(const RHICommandList* commandList, RHIFence* fence, std::span<RHISemaphore*> waitSemaphores,
                            std::span<RHISemaphore*> signalSemaphores) {
    if (!m_CommandQueue) { return; }

    // Wait on semaphores
    for (auto* semaphore: waitSemaphores) {
        auto* dx12Semaphore = static_cast<DirectX12Semaphore*>(semaphore);
        if (dx12Semaphore) { Wait(dx12Semaphore, dx12Semaphore->GetLastSignaledValue()); }
    }

    // Execute command list
    auto* dx12CmdList = static_cast<const DirectX12CommandList*>(commandList);
    if (dx12CmdList && dx12CmdList->GetCommandList()) {
        ID3D12CommandList* cmdLists[] = {dx12CmdList->GetCommandList()};
        m_CommandQueue->ExecuteCommandLists(1, cmdLists);
    }

    // Signal semaphores
    for (auto* semaphore: signalSemaphores) {
        auto* dx12Semaphore = static_cast<DirectX12Semaphore*>(semaphore);
        if (dx12Semaphore) { Signal(dx12Semaphore); }
    }

    // Signal fence
    auto* dx12Fence = static_cast<DirectX12Fence*>(fence);
    if (dx12Fence) { Signal(dx12Fence, dx12Fence->GetNextValue()); }
}

void DirectX12Queue::SubmitCommandLists(std::span<const RHICommandList*> commandLists, RHIFence* signalFence) {
    if (commandLists.empty() || !m_CommandQueue) { return; }

    std::vector<ID3D12CommandList*> d3dCommandLists;
    d3dCommandLists.reserve(commandLists.size());

    for (auto* cmdList: commandLists) {
        auto* dx12CmdList = static_cast<const DirectX12CommandList*>(cmdList);
        if (dx12CmdList && dx12CmdList->GetCommandList()) { d3dCommandLists.push_back(dx12CmdList->GetCommandList()); }
    }

    if (!d3dCommandLists.empty()) {
        m_CommandQueue->ExecuteCommandLists(static_cast<UINT>(d3dCommandLists.size()), d3dCommandLists.data());
    }

    // Signal fence if provided
    if (signalFence) {
        auto* dx12Fence = static_cast<DirectX12Fence*>(signalFence);
        if (dx12Fence) { Signal(dx12Fence, dx12Fence->GetNextValue()); }
    }
}

void DirectX12Queue::WaitIdle() {
    if (!m_CommandQueue || !m_InternalFence) { return; }

    uint64 fenceValue = m_InternalFence->GetNextValue();

    HRESULT hr = m_CommandQueue->Signal(m_InternalFence->GetFence(), fenceValue);
    if (FAILED(hr)) { Internal::LogError("DirectX12Queue: Failed to signal fence for WaitIdle"); }

    m_InternalFence->Wait(fenceValue);
}

void DirectX12Queue::Signal(DirectX12Fence* fence, uint64 value) {
    if (!m_CommandQueue || !fence || !fence->GetFence()) { return; }

    HRESULT hr = m_CommandQueue->Signal(fence->GetFence(), value);
    if (FAILED(hr)) { Internal::LogError("DirectX12Queue: Failed to signal fence from GPU"); }
}

void DirectX12Queue::Signal(DirectX12Semaphore* semaphore) {
    if (!m_CommandQueue || !semaphore || !semaphore->GetFence()) { return; }

    uint64 signalValue = semaphore->GetNextSignalValue();
    HRESULT hr = m_CommandQueue->Signal(semaphore->GetFence(), signalValue);
    if (FAILED(hr)) { Internal::LogError("DirectX12Queue: Failed to signal semaphore from GPU"); }
}

void DirectX12Queue::Wait(DirectX12Fence* fence, uint64 value) {
    if (!m_CommandQueue || !fence || !fence->GetFence()) { return; }

    HRESULT hr = m_CommandQueue->Wait(fence->GetFence(), value);
    if (FAILED(hr)) { Internal::LogError("DirectX12Queue: Failed to wait on fence from GPU"); }
}

void DirectX12Queue::Wait(DirectX12Semaphore* semaphore, uint64 value) {
    if (!m_CommandQueue || !semaphore || !semaphore->GetFence()) { return; }

    HRESULT hr = m_CommandQueue->Wait(semaphore->GetFence(), value);
    if (FAILED(hr)) { Internal::LogError("DirectX12Queue: Failed to wait on semaphore from GPU"); }
}

uint64 DirectX12Queue::ExecuteCommandLists(const std::vector<ID3D12CommandList*>& commandLists) {
    if (commandLists.empty() || !m_CommandQueue) { return 0; }

    m_CommandQueue->ExecuteCommandLists(static_cast<UINT>(commandLists.size()), commandLists.data());

    // Signal the internal fence and return its value for external synchronization
    uint64 fenceValue = m_InternalFence->GetNextValue();
    m_CommandQueue->Signal(m_InternalFence->GetFence(), fenceValue);

    return fenceValue;
}

} // namespace iGe
#endif
