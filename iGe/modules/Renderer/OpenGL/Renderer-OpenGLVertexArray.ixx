module;
#include "iGeMacro.h"

export module iGe.Renderer:OpenGLVertexArray;
import :VertexArray;
import :Buffer;
import iGe.Common;

namespace iGe
{
export class IGE_API OpenGLVertexArray : public VertexArray {
public:
    OpenGLVertexArray();
    virtual ~OpenGLVertexArray() override;

    virtual void Bind() const override;
    virtual void Unbind() const override;

    virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
    virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override;

    virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const override { return m_VertexBuffers; }
    virtual const Ref<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }

private:
    uint32 m_RendererID;
    uint32 m_VertexBufferIndex = 0;
    std::vector<Ref<VertexBuffer>> m_VertexBuffers;
    Ref<IndexBuffer> m_IndexBuffer;
};
} // namespace iGe
