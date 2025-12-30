module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #include <d3d12.h>
    #include <wrl/client.h>

export module iGe.RHI:DirectX12Buffer;
import :RHIBuffer;
import iGe.Common;

namespace iGe
{

export class IGE_API DirectX12Buffer : public RHIBuffer {
public:
    DirectX12Buffer(ID3D12Device* device, const RHIBufferCreateInfo& info);
    ~DirectX12Buffer() override;

    void* GetNativeHandle() const override { return m_Resource.Get(); }

    // RHIBuffer interface
    void* Map() override;
    void Unmap() override;
    bool IsMapped() const override { return m_MappedData != nullptr; }
    void Update(uint64 offset, uint64 size, const void* data) override;
    void Flush(uint64 offset = 0, uint64 size = ~0ULL) override;
    void Invalidate(uint64 offset = 0, uint64 size = ~0ULL) override;

    // DirectX12 specific
    ID3D12Resource* GetResource() const { return m_Resource.Get(); }
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return m_Resource->GetGPUVirtualAddress(); }

protected:
    void CreateResource(ID3D12Device* device, D3D12_HEAP_TYPE heapType, D3D12_RESOURCE_STATES initialState,
                        uint64 size);

    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
    void* m_MappedData = nullptr;
};

export class IGE_API DirectX12VertexBuffer : public RHIVertexBuffer {
public:
    DirectX12VertexBuffer(ID3D12Device* device, const RHIVertexBufferCreateInfo& info);
    ~DirectX12VertexBuffer() override;

    void* GetNativeHandle() const override { return m_Resource.Get(); }

    // RHIBuffer interface
    void* Map() override;
    void Unmap() override;
    bool IsMapped() const override { return m_MappedData != nullptr; }
    void Update(uint64 offset, uint64 size, const void* data) override;
    void Flush(uint64 offset = 0, uint64 size = ~0ULL) override;
    void Invalidate(uint64 offset = 0, uint64 size = ~0ULL) override;

    // DirectX12 specific
    ID3D12Resource* GetResource() const { return m_Resource.Get(); }
    const D3D12_VERTEX_BUFFER_VIEW& GetView() const { return m_View; }
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return m_Resource->GetGPUVirtualAddress(); }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
    D3D12_VERTEX_BUFFER_VIEW m_View{};
    void* m_MappedData = nullptr;
};

export class IGE_API DirectX12IndexBuffer : public RHIIndexBuffer {
public:
    DirectX12IndexBuffer(ID3D12Device* device, const RHIIndexBufferCreateInfo& info);
    ~DirectX12IndexBuffer() override;

    void* GetNativeHandle() const override { return m_Resource.Get(); }

    // RHIBuffer interface
    void* Map() override;
    void Unmap() override;
    bool IsMapped() const override { return m_MappedData != nullptr; }
    void Update(uint64 offset, uint64 size, const void* data) override;
    void Flush(uint64 offset = 0, uint64 size = ~0ULL) override;
    void Invalidate(uint64 offset = 0, uint64 size = ~0ULL) override;

    // DirectX12 specific
    ID3D12Resource* GetResource() const { return m_Resource.Get(); }
    const D3D12_INDEX_BUFFER_VIEW& GetView() const { return m_View; }
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return m_Resource->GetGPUVirtualAddress(); }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
    D3D12_INDEX_BUFFER_VIEW m_View{};
    void* m_MappedData = nullptr;
};

export class IGE_API DirectX12UniformBuffer : public RHIUniformBuffer {
public:
    DirectX12UniformBuffer(ID3D12Device* device, const RHIUniformBufferCreateInfo& info);
    ~DirectX12UniformBuffer() override;

    void* GetNativeHandle() const override { return m_Resource.Get(); }

    // RHIBuffer interface
    void* Map() override;
    void Unmap() override;
    bool IsMapped() const override { return m_MappedData != nullptr; }
    void Update(uint64 offset, uint64 size, const void* data) override;
    void Flush(uint64 offset = 0, uint64 size = ~0ULL) override;
    void Invalidate(uint64 offset = 0, uint64 size = ~0ULL) override;

    // DirectX12 specific
    ID3D12Resource* GetResource() const { return m_Resource.Get(); }
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return m_Resource->GetGPUVirtualAddress(); }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
    void* m_MappedData = nullptr;
};

export class IGE_API DirectX12StorageBuffer : public RHIStorageBuffer {
public:
    DirectX12StorageBuffer(ID3D12Device* device, const RHIStorageBufferCreateInfo& info);
    ~DirectX12StorageBuffer() override;

    void* GetNativeHandle() const override { return m_Resource.Get(); }

    // RHIBuffer interface
    void* Map() override;
    void Unmap() override;
    bool IsMapped() const override { return m_MappedData != nullptr; }
    void Update(uint64 offset, uint64 size, const void* data) override;
    void Flush(uint64 offset = 0, uint64 size = ~0ULL) override;
    void Invalidate(uint64 offset = 0, uint64 size = ~0ULL) override;

    // DirectX12 specific
    ID3D12Resource* GetResource() const { return m_Resource.Get(); }
    D3D12_GPU_VIRTUAL_ADDRESS GetGpuAddress() const { return m_Resource->GetGPUVirtualAddress(); }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_Resource;
    void* m_MappedData = nullptr;
};

} // namespace iGe
#endif
