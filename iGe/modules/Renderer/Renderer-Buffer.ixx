module;
#include "iGeMacro.h"

export module iGe.Renderer:Buffer;
import iGe.Common;

namespace iGe
{
export enum class ShaderDataType : int {
    None = 0,
    Float,
    Float2,
    Float3,
    Float4,
    Mat3,
    Mat4,
    Int,
    Int2,
    Int3,
    Int4,
    Bool
};

export struct IGE_API BufferElement {
    string Name;
    ShaderDataType Type;
    uint32 Size;
    uint64 Offset;
    bool Normalized;

    BufferElement() = default;

    BufferElement(ShaderDataType type, const string& name, bool normalized = false);
    uint32 GetComponentCount() const;
};

export class IGE_API BufferLayout {
public:
    BufferLayout();
    BufferLayout(std::initializer_list<BufferElement> elements);

    uint32 GetStride() const;
    const std::vector<BufferElement>& GetElements() const;

    auto elements() noexcept { return std::views::all(m_Elements); }
    auto elements() const noexcept { return std::views::all(m_Elements); }

private:
    void CalculateOffsetsAndStride();

    std::vector<BufferElement> m_Elements;
    uint32 m_Stride = 0;
};

export class IGE_API VertexBuffer {
public:
    virtual ~VertexBuffer() = default;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    virtual void SetLayout(const BufferLayout& layout) = 0;
    virtual const BufferLayout& GetLayout() const = 0;

    //static Ref<VertexBuffer> Create(uint32 size);
    static Ref<VertexBuffer> Create(float32* vertices, uint32 size);
};

// Currently IGE only supports 32-bit index buffers
export class IGE_API IndexBuffer {
public:
    virtual ~IndexBuffer() = default;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    virtual uint32 GetCount() const = 0;

    static Ref<IndexBuffer> Create(uint32* indices, uint32 count);
};

export class IGE_API UniformBuffer {
public:
    virtual ~UniformBuffer() = default;

    virtual void Bind(uint32 bindingPoint) const = 0;
    virtual void Unbind() const = 0;

    virtual void SetData(const void* data, uint32 size, uint32 offset = 0) const = 0;

    static Ref<UniformBuffer> Create(const void* data, uint32 size);
};
} // namespace iGe
