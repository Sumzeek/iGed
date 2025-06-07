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

export class IGE_API OpenGLBuffer : public Buffer {
public:
    OpenGLBuffer(const void* data, uint32_t size);
    virtual ~OpenGLBuffer();

    virtual void SetData(const void* data, uint32_t size, uint32_t offset = 0) override;
    virtual void GetData(void* data, uint32_t size, uint32_t offset = 0) override;

    virtual void Bind(uint32_t slot, BufferType type) override;
    virtual void Unbind() const override;

private:
    uint32_t m_RendererID;
    uint32_t m_Target;
};

} // namespace iGe
