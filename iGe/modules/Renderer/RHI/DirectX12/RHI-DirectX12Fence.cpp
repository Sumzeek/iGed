module;
#if defined(IGE_PLATFORM_WINDOWS)
    #define NOMINMAX
    #include <d3d12.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12Fence;

namespace iGe
{

// =================================================================================================
// DirectX12Fence
// =================================================================================================

DirectX12Fence::DirectX12Fence(ID3D12Device* device, const RHIFenceCreateInfo& info) : RHIFence(info) {
    // Initialize the fence with the specified initial value
    Initialize(device, info.InitialValue);

    // If signaled is requested, set the signaled value to the initial value
    // so that IsSignaled() returns true and Wait() returns immediately
    if (info.Signaled) {
        m_SignaledValue = info.InitialValue;
    } else {
        // If not signaled, we expect a signal to occur, so set signaled value
        // to the next expected value
        m_SignaledValue = info.InitialValue + 1;
    }
}

void DirectX12Fence::Initialize(ID3D12Device* device, uint64 initialValue) {
    if (!device) { Internal::LogError("DirectX12Fence: Device is null"); }

    m_NextValue = initialValue;

    // Create the fence
    HRESULT hr = device->CreateFence(initialValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));

    if (FAILED(hr)) { Internal::LogError("DirectX12Fence: Failed to create fence"); }

    // Create an event handle for CPU-side waiting
    m_FenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!m_FenceEvent) { Internal::LogError("DirectX12Fence: Failed to create fence event"); }
}

DirectX12Fence::~DirectX12Fence() {
    if (m_FenceEvent) {
        CloseHandle(m_FenceEvent);
        m_FenceEvent = nullptr;
    }
}

bool DirectX12Fence::Wait(uint64 timeout) {
    if (!m_Fence || !m_FenceEvent) { return false; }

    // Check if fence has already been signaled
    if (m_Fence->GetCompletedValue() >= m_SignaledValue) { return true; }

    // Set up event to be signaled when fence reaches the signaled value
    HRESULT hr = m_Fence->SetEventOnCompletion(m_SignaledValue, m_FenceEvent);
    if (FAILED(hr)) { return false; }

    // Convert timeout to milliseconds (input is nanoseconds)
    DWORD timeoutMs =
            (timeout == std::numeric_limits<uint64>::max()) ? INFINITE : static_cast<DWORD>(timeout / 1000000);

    // Wait for the event
    DWORD waitResult = WaitForSingleObject(m_FenceEvent, timeoutMs);
    return (waitResult == WAIT_OBJECT_0);
}

void DirectX12Fence::WaitForValue(uint64 value, uint32 timeoutMs) {
    if (!m_Fence || !m_FenceEvent) { return; }

    // Check if fence has already reached the value
    if (m_Fence->GetCompletedValue() >= value) { return; }

    // Set up event to be signaled when fence reaches value
    HRESULT hr = m_Fence->SetEventOnCompletion(value, m_FenceEvent);
    if (FAILED(hr)) { Internal::LogError("DirectX12Fence: Failed to set event on completion"); }

    // Wait for the event
    WaitForSingleObject(m_FenceEvent, timeoutMs);
}

void DirectX12Fence::Reset() {
    // In D3D12, we reset by incrementing the signaled value
    m_SignaledValue = m_NextValue + 1;
}

bool DirectX12Fence::IsSignaled() const {
    if (!m_Fence) { return false; }
    return m_Fence->GetCompletedValue() >= m_SignaledValue;
}

void DirectX12Fence::Signal(uint64 value) {
    if (m_Fence) {
        HRESULT hr = m_Fence->Signal(value);
        if (FAILED(hr)) { Internal::LogError("DirectX12Fence: Failed to signal fence from CPU"); }
        m_NextValue = value;
    }
}

uint64 DirectX12Fence::GetCompletedValue() const {
    if (m_Fence) { return m_Fence->GetCompletedValue(); }
    return 0;
}

} // namespace iGe
#endif
