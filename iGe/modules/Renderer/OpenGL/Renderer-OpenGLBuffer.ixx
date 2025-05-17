module;
#include "iGeMacro.h"

export module iGe.Renderer:OpenGLBuffer;
import :Buffer;
import std;

namespace iGe
{

export class IGE_API OpenGLVertexBuffer : public VertexBuffer {
public:
    OpenGLVertexBuffer(float* vertices, uint32_t size);
    virtual ~OpenGLVertexBuffer();

    virtual void SetLayout(const BufferLayout& layout) override;
    virtual const BufferLayout& GetLayout() const override;

    virtual void Bind() const override;
    virtual void Unbind() const override;

private:
    uint32_t m_RendererID;
    BufferLayout m_Layout;
};

export class IGE_API OpenGLIndexBuffer : public IndexBuffer {
public:
    OpenGLIndexBuffer(uint32_t* indices, uint32_t count);
    virtual ~OpenGLIndexBuffer();

    virtual uint32_t GetCount() const override;

    virtual void Bind() const override;
    virtual void Unbind() const override;

private:
    uint32_t m_RendererID;
    uint32_t m_Count;
};

export class IGE_API OpenGLUniformBuffer : public UniformBuffer {
public:
    OpenGLUniformBuffer(const void* data, uint32_t size);
    virtual ~OpenGLUniformBuffer();

    virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;

    virtual void Bind(uint32_t bindingPoint) const override;
    virtual void Unbind() const override;

private:
    uint32_t m_RendererID;
};

} // namespace iGe
