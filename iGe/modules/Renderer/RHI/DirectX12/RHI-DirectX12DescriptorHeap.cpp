module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <mutex>
    #include <queue>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12DescriptorHeap;

namespace iGe
{

// =================================================================================================
// DirectX12DescriptorHeapAllocator Implementation
// =================================================================================================

void DirectX12DescriptorHeapAllocator::Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type,
                                                  uint32 numDescriptors, bool shaderVisible) {
    m_Type = type;
    m_NumDescriptors = numDescriptors;
    m_ShaderVisible = shaderVisible;
    m_DescriptorSize = device->GetDescriptorHandleIncrementSize(type);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = numDescriptors;
    heapDesc.Type = type;
    heapDesc.Flags = shaderVisible ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 0;

    HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_Heap));
    if (FAILED(hr)) {
        Internal::LogError("Failed to create descriptor heap");
        return;
    }

    m_CPUHeapStart = m_Heap->GetCPUDescriptorHandleForHeapStart();
    if (shaderVisible) { m_GPUHeapStart = m_Heap->GetGPUDescriptorHandleForHeapStart(); }

    m_NextFreeIndex = 0;
}

uint32 DirectX12DescriptorHeapAllocator::Allocate() {
    std::lock_guard<std::mutex> lock(m_Mutex);

    if (!m_FreeIndices.empty()) {
        uint32 index = m_FreeIndices.front();
        m_FreeIndices.pop();
        return index;
    }

    if (m_NextFreeIndex >= m_NumDescriptors) {
        Internal::LogError("Descriptor heap out of space");
        return UINT32_MAX;
    }

    return m_NextFreeIndex++;
}

uint32 DirectX12DescriptorHeapAllocator::AllocateRange(uint32 count) {
    std::lock_guard<std::mutex> lock(m_Mutex);

    // For simplicity, only allocate from the end (no fragmentation handling for ranges)
    if (m_NextFreeIndex + count > m_NumDescriptors) {
        Internal::LogError("Descriptor heap out of space for range allocation");
        return UINT32_MAX;
    }

    uint32 startIndex = m_NextFreeIndex;
    m_NextFreeIndex += count;
    return startIndex;
}

void DirectX12DescriptorHeapAllocator::Free(uint32 index) {
    if (index == UINT32_MAX) return;

    std::lock_guard<std::mutex> lock(m_Mutex);
    m_FreeIndices.push(index);
}

void DirectX12DescriptorHeapAllocator::FreeRange(uint32 startIndex, uint32 count) {
    if (startIndex == UINT32_MAX) return;

    std::lock_guard<std::mutex> lock(m_Mutex);
    for (uint32 i = 0; i < count; ++i) { m_FreeIndices.push(startIndex + i); }
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectX12DescriptorHeapAllocator::GetCPUHandle(uint32 index) const {
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_CPUHeapStart;
    handle.ptr += static_cast<SIZE_T>(index) * m_DescriptorSize;
    return handle;
}

D3D12_GPU_DESCRIPTOR_HANDLE DirectX12DescriptorHeapAllocator::GetGPUHandle(uint32 index) const {
    if (!m_ShaderVisible) { return {}; }
    D3D12_GPU_DESCRIPTOR_HANDLE handle = m_GPUHeapStart;
    handle.ptr += static_cast<UINT64>(index) * m_DescriptorSize;
    return handle;
}

void DirectX12DescriptorHeapAllocator::Reset() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_NextFreeIndex = 0;
    while (!m_FreeIndices.empty()) { m_FreeIndices.pop(); }
}

// =================================================================================================
// DirectX12StagingDescriptorHeap Implementation
// =================================================================================================

void DirectX12StagingDescriptorHeap::Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type,
                                                uint32 numDescriptors) {
    m_Type = type;
    m_NumDescriptors = numDescriptors;
    m_DescriptorSize = device->GetDescriptorHandleIncrementSize(type);

    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = numDescriptors;
    heapDesc.Type = type;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE; // Non-shader visible for staging
    heapDesc.NodeMask = 0;

    HRESULT hr = device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_Heap));
    if (FAILED(hr)) {
        Internal::LogError("Failed to create staging descriptor heap");
        return;
    }

    m_CPUHeapStart = m_Heap->GetCPUDescriptorHandleForHeapStart();
    m_NextFreeIndex = 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE DirectX12StagingDescriptorHeap::Allocate() {
    std::lock_guard<std::mutex> lock(m_Mutex);

    uint32 index;
    if (!m_FreeIndices.empty()) {
        index = m_FreeIndices.front();
        m_FreeIndices.pop();
    } else {
        if (m_NextFreeIndex >= m_NumDescriptors) {
            Internal::LogError("Staging descriptor heap out of space");
            return {};
        }
        index = m_NextFreeIndex++;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_CPUHeapStart;
    handle.ptr += static_cast<SIZE_T>(index) * m_DescriptorSize;
    return handle;
}

void DirectX12StagingDescriptorHeap::Free(D3D12_CPU_DESCRIPTOR_HANDLE handle) {
    if (handle.ptr == 0) return;

    std::lock_guard<std::mutex> lock(m_Mutex);
    uint32 index = static_cast<uint32>((handle.ptr - m_CPUHeapStart.ptr) / m_DescriptorSize);
    m_FreeIndices.push(index);
}

void DirectX12StagingDescriptorHeap::Reset() {
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_NextFreeIndex = 0;
    while (!m_FreeIndices.empty()) { m_FreeIndices.pop(); }
}

} // namespace iGe
#endif
