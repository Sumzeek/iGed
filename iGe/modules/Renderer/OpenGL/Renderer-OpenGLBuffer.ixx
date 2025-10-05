module;
#include "iGeMacro.h"

export module iGe.Renderer:OpenGLBuffer;
import :Buffer;
import iGe.Common;

namespace iGe
{
export class IGE_API OpenGLVertexBuffer : public VertexBuffer {
public:
    OpenGLVertexBuffer(float* vertices, uint32 size);
    virtual ~OpenGLVertexBuffer();

    virtual void SetLayout(const BufferLayout& layout) override;
    virtual const BufferLayout& GetLayout() const override;

    virtual void Bind() const override;
    virtual void Unbind() const override;

private:
    uint32 m_RendererID;
    BufferLayout m_Layout;
};

export class IGE_API OpenGLIndexBuffer : public IndexBuffer {
public:
    OpenGLIndexBuffer(uint32* indices, uint32 count);
    virtual ~OpenGLIndexBuffer();

    virtual uint32 GetCount() const override;

    virtual void Bind() const override;
    virtual void Unbind() const override;

private:
    uint32 m_RendererID;
    uint32 m_Count;
};

export class IGE_API OpenGLUniformBuffer : public UniformBuffer {
public:
    OpenGLUniformBuffer(const void* data, uint32 size);
    virtual ~OpenGLUniformBuffer();

    virtual void SetData(const void* data, uint32 size, uint32 offset = 0) const override;

    virtual void Bind(uint32 bindingPoint) const override;
    virtual void Unbind() const override;

private:
    uint32 m_RendererID;
};
} // namespace iGe
