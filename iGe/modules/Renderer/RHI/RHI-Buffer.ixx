module;
#include "iGeMacro.h"

export module iGe.RHI:RHIBuffer;
import :RHIResource;
import iGe.Common;

namespace iGe
{

export enum class RHIBufferUsageBit : uint32 {
    None = 0,
    VertexBuffer = 1 << 0,
    IndexBuffer = 1 << 1,
    UniformBuffer = 1 << 2,
    StorageBuffer = 1 << 3,
    IndirectBuffer = 1 << 4,
    TransferSrc = 1 << 5,
    TransferDst = 1 << 6
};

export struct RHIBufferCreateInfo {
    uint64 Size;
    Flags<RHIBufferUsageBit> Usage;
    RHIMemoryUsage MemoryUsage;
};

export class RHIBuffer : public RHIResource {
public:
    ~RHIBuffer() override = default;

    uint64 GetSize() const { return m_Size; }
    RHIMemoryUsage GetMemoryUsage() const { return m_MemoryUsage; }
    Flags<RHIBufferUsageBit> GetUsage() const { return m_Usage; }

    // ==========================================================================
    // Buffer Operations (implemented by derived classes)
    // ==========================================================================

    // Map buffer memory for CPU access
    // Returns nullptr if mapping fails or buffer is not host-visible
    virtual void* Map() = 0;

    // Unmap previously mapped buffer memory
    virtual void Unmap() = 0;

    // Check if buffer is currently mapped
    virtual bool IsMapped() const = 0;

    // Update buffer data (handles mapping internally if needed)
    // For non-host-visible buffers, this may use staging buffers
    virtual void Update(uint64 offset, uint64 size, const void* data) = 0;

    // Convenience template for updating entire buffer
    template<typename T>
    void Update(const T& data) {
        Update(0, sizeof(T), &data);
    }

    // Convenience template for updating array data
    template<typename T>
    void Update(const std::vector<T>& data) {
        Update(0, data.size() * sizeof(T), data.data());
    }

    // Flush mapped memory range (for non-coherent memory)
    virtual void Flush(uint64 offset = 0, uint64 size = ~0ULL) = 0;

    // Invalidate mapped memory range (for non-coherent memory)
    virtual void Invalidate(uint64 offset = 0, uint64 size = ~0ULL) = 0;

protected:
    RHIBuffer(const RHIBufferCreateInfo& info)
        : RHIResource(RHIResourceType::Buffer), m_Size(info.Size), m_Usage(info.Usage),
          m_MemoryUsage(info.MemoryUsage) {}

    uint64 m_Size;
    Flags<RHIBufferUsageBit> m_Usage;
    RHIMemoryUsage m_MemoryUsage;
};

// =================================================================================================
// Vertex Buffer
// =================================================================================================

export struct RHIVertexBufferCreateInfo {
    uint64 Size;
    uint64 Stride;
    RHIMemoryUsage MemoryUsage = RHIMemoryUsage::CpuToGpu;
};

export class IGE_API RHIVertexBuffer : public RHIBuffer {
public:
    ~RHIVertexBuffer() override = default;

    uint64 GetStride() const { return m_Stride; }
    uint64 GetCount() const { return m_Size / m_Stride; }

protected:
    RHIVertexBuffer(const RHIVertexBufferCreateInfo& info)
        : RHIBuffer({info.Size, RHIBufferUsageBit::VertexBuffer | RHIBufferUsageBit::TransferDst, info.MemoryUsage}),
          m_Stride(info.Stride) {}

    uint64 m_Stride;
};

// =================================================================================================
// Index Buffer
// =================================================================================================

export enum class RHIIndexFormat { Uint16, Uint32 };

export struct RHIIndexBufferCreateInfo {
    uint64 Size;
    RHIIndexFormat Format = RHIIndexFormat::Uint32;
    RHIMemoryUsage MemoryUsage = RHIMemoryUsage::CpuToGpu;
};

export class IGE_API RHIIndexBuffer : public RHIBuffer {
public:
    ~RHIIndexBuffer() override = default;

    uint64 GetCount() const {
        uint32 stride = (m_Format == RHIIndexFormat::Uint16) ? 2 : 4;
        return m_Size / stride;
    }

    RHIIndexFormat GetFormat() const { return m_Format; }

protected:
    RHIIndexBuffer(const RHIIndexBufferCreateInfo& info)
        : RHIBuffer({info.Size, RHIBufferUsageBit::IndexBuffer | RHIBufferUsageBit::TransferDst, info.MemoryUsage}),
          m_Format(info.Format) {}

    RHIIndexFormat m_Format;
};

// =================================================================================================
// Uniform Buffer Layout
// =================================================================================================

export enum class UBElementType : uint32 {
    Float = 0,
    Float2,
    Float3,
    Float4,
    Int,
    Int2,
    Int3,
    Int4,
    Uint,
    Uint2,
    Uint3,
    Uint4,
    Bool,
    Float4x4,
};

export struct UBElement {
public:
    std::string Name;
    UBElementType Type;
    uint32 Offset;
    uint32 Size;

    UBElement() = default;
    UBElement(UBElementType type, const std::string& name);
};

export struct UniformBufferLayout {
public:
    UniformBufferLayout() = default;
    UniformBufferLayout(std::initializer_list<UBElement> elements);

    uint32 GetSize() const { return Size; }

private:
    void CalculateOffsets();

    std::string Name;
    uint32 Size = 0;
    std::vector<UBElement> Elements;
};

// =================================================================================================
// Uniform Buffer
// =================================================================================================

export struct RHIUniformBufferCreateInfo {
    UniformBufferLayout Layout;
    RHIMemoryUsage MemoryUsage = RHIMemoryUsage::CpuToGpu;
};

export class IGE_API RHIUniformBuffer : public RHIBuffer {
public:
    ~RHIUniformBuffer() override = default;

protected:
    RHIUniformBuffer(const RHIUniformBufferCreateInfo& info)
        : RHIBuffer({info.Layout.GetSize(), RHIBufferUsageBit::UniformBuffer | RHIBufferUsageBit::TransferDst,
                     info.MemoryUsage}) {}
};

// =================================================================================================
// Storage Buffer
// =================================================================================================

export struct RHIStorageBufferCreateInfo {
    uint64 Size;
    RHIMemoryUsage MemoryUsage = RHIMemoryUsage::GpuOnly;
    bool AllowRead = true;
    bool AllowWrite = true;
};

export class IGE_API RHIStorageBuffer : public RHIBuffer {
public:
    ~RHIStorageBuffer() override = default;

protected:
    RHIStorageBuffer(const RHIStorageBufferCreateInfo& info)
        : RHIBuffer({info.Size,
                     RHIBufferUsageBit::StorageBuffer | RHIBufferUsageBit::TransferSrc | RHIBufferUsageBit::TransferDst,
                     info.MemoryUsage}) {}
};

} // namespace iGe
