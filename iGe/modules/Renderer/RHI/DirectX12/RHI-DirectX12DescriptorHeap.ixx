module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <mutex>
    #include <queue>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12DescriptorHeap;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// Descriptor Heap Allocator
// Manages allocation of descriptors from a D3D12 descriptor heap
// =================================================================================================

export class IGE_API DirectX12DescriptorHeapAllocator {
public:
    DirectX12DescriptorHeapAllocator() = default;
    ~DirectX12DescriptorHeapAllocator() = default;

    void Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptors, bool shaderVisible);

    // Allocate a single descriptor, returns the index
    uint32 Allocate();

    // Allocate a contiguous range of descriptors, returns the starting index
    uint32 AllocateRange(uint32 count);

    // Free a single descriptor
    void Free(uint32 index);

    // Free a range of descriptors
    void FreeRange(uint32 startIndex, uint32 count);

    // Get handles
    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32 index) const;
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32 index) const;

    // Get heap
    ID3D12DescriptorHeap* GetHeap() const { return m_Heap.Get(); }
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> GetHeapComPtr() const { return m_Heap; }

    // Get descriptor size
    uint32 GetDescriptorSize() const { return m_DescriptorSize; }

    // Check if shader visible
    bool IsShaderVisible() const { return m_ShaderVisible; }

    // Reset all allocations
    void Reset();

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;
    D3D12_DESCRIPTOR_HEAP_TYPE m_Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    uint32 m_DescriptorSize = 0;
    uint32 m_NumDescriptors = 0;
    bool m_ShaderVisible = false;

    D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHeapStart = {};
    D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHeapStart = {};

    // Simple free list allocator
    std::mutex m_Mutex;
    std::queue<uint32> m_FreeIndices;
    uint32 m_NextFreeIndex = 0;
};

// =================================================================================================
// Staging Descriptor Heap
// For CPU-only staging heaps that copy to GPU-visible heaps
// =================================================================================================

export class IGE_API DirectX12StagingDescriptorHeap {
public:
    DirectX12StagingDescriptorHeap() = default;
    ~DirectX12StagingDescriptorHeap() = default;

    void Initialize(ID3D12Device* device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32 numDescriptors);

    // Allocate and get CPU handle
    D3D12_CPU_DESCRIPTOR_HANDLE Allocate();
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE handle);

    // Reset all allocations
    void Reset();

    ID3D12DescriptorHeap* GetHeap() const { return m_Heap.Get(); }
    uint32 GetDescriptorSize() const { return m_DescriptorSize; }

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_Heap;
    D3D12_DESCRIPTOR_HEAP_TYPE m_Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    uint32 m_DescriptorSize = 0;
    uint32 m_NumDescriptors = 0;

    D3D12_CPU_DESCRIPTOR_HANDLE m_CPUHeapStart = {};

    std::mutex m_Mutex;
    std::queue<uint32> m_FreeIndices;
    uint32 m_NextFreeIndex = 0;
};

// =================================================================================================
// Descriptor Handle
// Wrapper around D3D12 descriptor handles
// =================================================================================================

export struct DirectX12DescriptorHandle {
    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle = {};
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle = {};
    uint32 HeapIndex = std::numeric_limits<uint32>::max();

    bool IsValid() const { return HeapIndex != std::numeric_limits<uint32>::max(); }
    bool IsNull() const { return CPUHandle.ptr == 0; }
};

} // namespace iGe
#endif
