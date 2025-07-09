module;
#include "iGeMacro.h"

export module iGe.Renderer:Buffer;

import std;
import iGe.SmartPointer;

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
    std::string Name;
    ShaderDataType Type;
    std::uint32_t Size;
    size_t Offset;
    bool Normalized;

    BufferElement() = default;

    BufferElement(ShaderDataType type, const std::string& name, bool normalized = false);

    std::uint32_t GetComponentCount() const;
};

export class IGE_API BufferLayout {
public:
    BufferLayout();
    BufferLayout(std::initializer_list<BufferElement> elements);

    std::uint32_t GetStride() const;
    const std::vector<BufferElement>& GetElements() const;

    std::vector<BufferElement>::iterator begin();
    std::vector<BufferElement>::iterator end();
    std::vector<BufferElement>::const_iterator begin() const;
    std::vector<BufferElement>::const_iterator end() const;

private:
    void CalculateOffsetsAndStride();

    std::vector<BufferElement> m_Elements;
    std::uint32_t m_Stride = 0;
};

export class IGE_API VertexBuffer {
public:
    virtual ~VertexBuffer() = default;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    //virtual void SetData(const void* data, uint32_t size) = 0;

    virtual void SetLayout(const BufferLayout& layout) = 0;
    virtual const BufferLayout& GetLayout() const = 0;

    //static Ref<VertexBuffer> Create(std::uint32_t size);
    static Ref<VertexBuffer> Create(float* vertices, std::uint32_t size);
};

// Currently IGE only supports 32-bit index buffers
export class IGE_API IndexBuffer {
public:
    virtual ~IndexBuffer() = default;

    virtual void Bind() const = 0;
    virtual void Unbind() const = 0;

    virtual std::uint32_t GetCount() const = 0;

    static Ref<IndexBuffer> Create(std::uint32_t* indices, std::uint32_t count);
};

export class IGE_API UniformBuffer {
public:
    virtual ~UniformBuffer() = default;

    virtual void Bind(uint32_t bindingPoint) const = 0;
    virtual void Unbind() const = 0;

    virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) = 0;

    static Ref<UniformBuffer> Create(const void* data, uint32_t size);
};

} // namespace iGe