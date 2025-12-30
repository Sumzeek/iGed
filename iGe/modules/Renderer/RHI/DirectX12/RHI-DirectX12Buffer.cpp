module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12Buffer;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// Static Method
// =================================================================================================

D3D12_HEAP_PROPERTIES GetHeapProperties(RHIMemoryUsage usage) {
    D3D12_HEAP_PROPERTIES props = {};
    props.Type = D3D12_HEAP_TYPE_DEFAULT;
    props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    props.CreationNodeMask = 1;
    props.VisibleNodeMask = 1;

    switch (usage) {
        case RHIMemoryUsage::GpuOnly:
            props.Type = D3D12_HEAP_TYPE_DEFAULT;
            break;
        case RHIMemoryUsage::CpuToGpu:
            props.Type = D3D12_HEAP_TYPE_UPLOAD;
            break;
        case RHIMemoryUsage::GpuToCpu:
            props.Type = D3D12_HEAP_TYPE_READBACK;
            break;
        default:
            props.Type = D3D12_HEAP_TYPE_DEFAULT;
            break;
    }
    return props;
}

D3D12_RESOURCE_STATES GetInitialState(RHIMemoryUsage usage) {
    switch (usage) {
        case RHIMemoryUsage::GpuOnly:
            return D3D12_RESOURCE_STATE_COMMON;
        case RHIMemoryUsage::CpuToGpu:
            return D3D12_RESOURCE_STATE_GENERIC_READ;
        case RHIMemoryUsage::GpuToCpu:
            return D3D12_RESOURCE_STATE_COPY_DEST;
        default:
            return D3D12_RESOURCE_STATE_COMMON;
    }
}

// =================================================================================================
// DirectX12Buffer
// =================================================================================================

DirectX12Buffer::DirectX12Buffer(ID3D12Device* device, const RHIBufferCreateInfo& info) : RHIBuffer(info) {
    auto props = GetHeapProperties(info.MemoryUsage);
    auto state = GetInitialState(info.MemoryUsage);

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = m_Size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    // Add UAV flag for storage buffers
    if (info.Usage.HasFlag(RHIBufferUsageBit::StorageBuffer)) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    HRESULT hr = device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr,
                                                 IID_PPV_ARGS(&m_Resource));

    if (FAILED(hr)) {
        Internal::LogError("Failed to create Buffer");
        return;
    }
}

DirectX12Buffer::~DirectX12Buffer() {
    if (m_MappedData) { Unmap(); }
}

void* DirectX12Buffer::Map() {
    if (m_MappedData) { return m_MappedData; }

    if (m_MemoryUsage == RHIMemoryUsage::GpuOnly) {
        Internal::LogError("Cannot map a GPU only buffer");
        return nullptr;
    }

    D3D12_RANGE readRange = {0, 0};
    if (m_MemoryUsage == RHIMemoryUsage::GpuToCpu) { readRange.End = m_Size; }

    HRESULT hr = m_Resource->Map(0, &readRange, &m_MappedData);
    if (FAILED(hr)) {
        Internal::LogError("Failed to map buffer");
        return nullptr;
    }

    return m_MappedData;
}

void DirectX12Buffer::Unmap() {
    if (!m_MappedData) { return; }

    D3D12_RANGE writtenRange = {0, m_Size};
    if (m_MemoryUsage == RHIMemoryUsage::GpuToCpu) {
        writtenRange.End = 0; // No writes for readback buffers
    }

    m_Resource->Unmap(0, &writtenRange);
    m_MappedData = nullptr;
}

void DirectX12Buffer::Update(uint64 offset, uint64 size, const void* data) {
    if (!data || size == 0) return;

    void* mapped = Map();
    if (mapped) {
        memcpy(static_cast<uint8*>(mapped) + offset, data, size);
        // Note: For upload heaps, we don't need to unmap after every update
        // but for safety and simplicity, we unmap here
        Unmap();
    }
}

void DirectX12Buffer::Flush(uint64 offset, uint64 size) {
    // D3D12 upload heaps are write-combined and don't require explicit flush
}

void DirectX12Buffer::Invalidate(uint64 offset, uint64 size) {
    // D3D12 readback heaps don't require explicit invalidate
}

void DirectX12Buffer::CreateResource(ID3D12Device* device, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState,
                                     uint64 size) {
    D3D12_HEAP_PROPERTIES props = {};
    props.Type = heapType;
    props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    props.CreationNodeMask = 1;
    props.VisibleNodeMask = 1;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, initialState, nullptr,
                                    IID_PPV_ARGS(&m_Resource));
}

// =================================================================================================
// DirectX12VertexBuffer
// =================================================================================================

DirectX12VertexBuffer::DirectX12VertexBuffer(ID3D12Device* device, const RHIVertexBufferCreateInfo& info)
    : RHIVertexBuffer(info) {
    auto props = GetHeapProperties(info.MemoryUsage);
    auto state = GetInitialState(info.MemoryUsage);

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = info.Size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr,
                                                 IID_PPV_ARGS(&m_Resource));

    if (FAILED(hr)) {
        Internal::LogError("Failed to create Vertex Buffer");
        return;
    }

    m_View.BufferLocation = m_Resource->GetGPUVirtualAddress();
    m_View.SizeInBytes = static_cast<UINT>(info.Size);
    m_View.StrideInBytes = static_cast<UINT>(info.Stride);
}

DirectX12VertexBuffer::~DirectX12VertexBuffer() {
    if (m_MappedData) { Unmap(); }
}

void* DirectX12VertexBuffer::Map() {
    if (m_MappedData) { return m_MappedData; }

    if (m_MemoryUsage == RHIMemoryUsage::GpuOnly) {
        Internal::LogError("Cannot map a GPU only buffer");
        return nullptr;
    }

    D3D12_RANGE readRange = {0, 0};
    HRESULT hr = m_Resource->Map(0, &readRange, &m_MappedData);
    if (FAILED(hr)) {
        Internal::LogError("Failed to map vertex buffer");
        return nullptr;
    }

    return m_MappedData;
}

void DirectX12VertexBuffer::Unmap() {
    if (!m_MappedData) { return; }

    m_Resource->Unmap(0, nullptr);
    m_MappedData = nullptr;
}

void DirectX12VertexBuffer::Update(uint64 offset, uint64 size, const void* data) {
    if (!data || size == 0) return;

    void* mapped = Map();
    if (mapped) {
        memcpy(static_cast<uint8*>(mapped) + offset, data, size);
        Unmap();
    }
}

void DirectX12VertexBuffer::Flush(uint64 offset, uint64 size) {
    // D3D12 upload heaps are write-combined and don't require explicit flush
}

void DirectX12VertexBuffer::Invalidate(uint64 offset, uint64 size) {
    // D3D12 readback heaps don't require explicit invalidate
}

// =================================================================================================
// DirectX12IndexBuffer
// =================================================================================================

DirectX12IndexBuffer::DirectX12IndexBuffer(ID3D12Device* device, const RHIIndexBufferCreateInfo& info)
    : RHIIndexBuffer(info) {
    auto props = GetHeapProperties(info.MemoryUsage);
    auto state = GetInitialState(info.MemoryUsage);

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = info.Size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr,
                                                 IID_PPV_ARGS(&m_Resource));

    if (FAILED(hr)) {
        Internal::LogError("Failed to create Index Buffer");
        return;
    }

    m_View.BufferLocation = m_Resource->GetGPUVirtualAddress();
    m_View.SizeInBytes = static_cast<UINT>(info.Size);
    m_View.Format = (info.Format == RHIIndexFormat::Uint16) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
}

DirectX12IndexBuffer::~DirectX12IndexBuffer() {
    if (m_MappedData) { Unmap(); }
}

void* DirectX12IndexBuffer::Map() {
    if (m_MappedData) { return m_MappedData; }

    if (m_MemoryUsage == RHIMemoryUsage::GpuOnly) {
        Internal::LogError("Cannot map a GPU only buffer");
        return nullptr;
    }

    D3D12_RANGE readRange = {0, 0};
    HRESULT hr = m_Resource->Map(0, &readRange, &m_MappedData);
    if (FAILED(hr)) {
        Internal::LogError("Failed to map index buffer");
        return nullptr;
    }

    return m_MappedData;
}

void DirectX12IndexBuffer::Unmap() {
    if (!m_MappedData) { return; }

    m_Resource->Unmap(0, nullptr);
    m_MappedData = nullptr;
}

void DirectX12IndexBuffer::Update(uint64 offset, uint64 size, const void* data) {
    if (!data || size == 0) return;

    void* mapped = Map();
    if (mapped) {
        memcpy(static_cast<uint8*>(mapped) + offset, data, size);
        Unmap();
    }
}

void DirectX12IndexBuffer::Flush(uint64 offset, uint64 size) {
    // D3D12 upload heaps are write-combined and don't require explicit flush
}

void DirectX12IndexBuffer::Invalidate(uint64 offset, uint64 size) {
    // D3D12 readback heaps don't require explicit invalidate
}

// =================================================================================================
// DirectX12UniformBuffer
// =================================================================================================

DirectX12UniformBuffer::DirectX12UniformBuffer(ID3D12Device* device, const RHIUniformBufferCreateInfo& info)
    : RHIUniformBuffer(info) {
    auto props = GetHeapProperties(info.MemoryUsage);
    auto state = GetInitialState(info.MemoryUsage);

    // Uniform buffer size must be 256-byte aligned
    uint64 alignedSize = (info.Layout.GetSize() + 255) & ~255;

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = alignedSize;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr,
                                                 IID_PPV_ARGS(&m_Resource));

    if (FAILED(hr)) {
        Internal::LogError("Failed to create Uniform Buffer");
        return;
    }
}

DirectX12UniformBuffer::~DirectX12UniformBuffer() {
    if (m_MappedData) { Unmap(); }
}

void* DirectX12UniformBuffer::Map() {
    if (m_MappedData) { return m_MappedData; }

    if (m_MemoryUsage == RHIMemoryUsage::GpuOnly) {
        Internal::LogError("Cannot map a GPU only buffer");
        return nullptr;
    }

    D3D12_RANGE readRange = {0, 0};
    HRESULT hr = m_Resource->Map(0, &readRange, &m_MappedData);
    if (FAILED(hr)) {
        Internal::LogError("Failed to map uniform buffer");
        return nullptr;
    }

    return m_MappedData;
}

void DirectX12UniformBuffer::Unmap() {
    if (!m_MappedData) { return; }

    m_Resource->Unmap(0, nullptr);
    m_MappedData = nullptr;
}

void DirectX12UniformBuffer::Update(uint64 offset, uint64 size, const void* data) {
    if (!data || size == 0) return;

    void* mapped = Map();
    if (mapped) {
        memcpy(static_cast<uint8*>(mapped) + offset, data, size);
        Unmap();
    }
}

void DirectX12UniformBuffer::Flush(uint64 offset, uint64 size) {
    // D3D12 upload heaps are write-combined and don't require explicit flush
}

void DirectX12UniformBuffer::Invalidate(uint64 offset, uint64 size) {
    // D3D12 readback heaps don't require explicit invalidate
}

// =================================================================================================
// DirectX12StorageBuffer
// =================================================================================================

DirectX12StorageBuffer::DirectX12StorageBuffer(ID3D12Device* device, const RHIStorageBufferCreateInfo& info)
    : RHIStorageBuffer(info) {
    auto props = GetHeapProperties(info.MemoryUsage);
    auto state = GetInitialState(info.MemoryUsage);

    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Alignment = 0;
    desc.Width = info.Size;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    HRESULT hr = device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, state, nullptr,
                                                 IID_PPV_ARGS(&m_Resource));

    if (FAILED(hr)) {
        Internal::LogError("Failed to create Storage Buffer");
        return;
    }
}

DirectX12StorageBuffer::~DirectX12StorageBuffer() {
    if (m_MappedData) { Unmap(); }
}

void* DirectX12StorageBuffer::Map() {
    if (m_MappedData) { return m_MappedData; }

    if (m_MemoryUsage == RHIMemoryUsage::GpuOnly) {
        Internal::LogError("Cannot map a GPU only buffer");
        return nullptr;
    }

    D3D12_RANGE readRange = {0, 0};
    if (m_MemoryUsage == RHIMemoryUsage::GpuToCpu) { readRange.End = m_Size; }

    HRESULT hr = m_Resource->Map(0, &readRange, &m_MappedData);
    if (FAILED(hr)) {
        Internal::LogError("Failed to map storage buffer");
        return nullptr;
    }

    return m_MappedData;
}

void DirectX12StorageBuffer::Unmap() {
    if (!m_MappedData) { return; }

    D3D12_RANGE writtenRange = {0, m_Size};
    if (m_MemoryUsage == RHIMemoryUsage::GpuToCpu) { writtenRange.End = 0; }

    m_Resource->Unmap(0, &writtenRange);
    m_MappedData = nullptr;
}

void DirectX12StorageBuffer::Update(uint64 offset, uint64 size, const void* data) {
    if (!data || size == 0) return;

    void* mapped = Map();
    if (mapped) {
        memcpy(static_cast<uint8*>(mapped) + offset, data, size);
        Unmap();
    }
}

void DirectX12StorageBuffer::Flush(uint64 offset, uint64 size) {
    // D3D12 upload heaps are write-combined and don't require explicit flush
}

void DirectX12StorageBuffer::Invalidate(uint64 offset, uint64 size) {
    // D3D12 readback heaps don't require explicit invalidate
}

} // namespace iGe
#endif
