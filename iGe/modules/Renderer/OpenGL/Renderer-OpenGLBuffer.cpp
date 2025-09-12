module;
#include <glad/gl.h>

module iGe.Renderer;
import :OpenGLBuffer;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// OpenGLVertexBuffer ///////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32 size) {
    glCreateBuffers(1, &m_RendererID);
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

OpenGLVertexBuffer::~OpenGLVertexBuffer() { glDeleteBuffers(1, &m_RendererID); }

void OpenGLVertexBuffer::SetLayout(const BufferLayout& layout) { m_Layout = layout; }

const BufferLayout& OpenGLVertexBuffer::GetLayout() const { return m_Layout; }

void OpenGLVertexBuffer::Bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_RendererID); }

void OpenGLVertexBuffer::Unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

/////////////////////////////////////////////////////////////////////////////
// OpenGLVIndexBuffer ///////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLIndexBuffer::OpenGLIndexBuffer(uint32* indices, uint32 count) : m_Count{count} {
    glCreateBuffers(1, &m_RendererID);

    // GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO
    // Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state.
    glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    glBufferData(GL_ARRAY_BUFFER, count * sizeof(uint32), indices, GL_STATIC_DRAW);
}

OpenGLIndexBuffer::~OpenGLIndexBuffer() { glDeleteBuffers(1, &m_RendererID); }

uint32 OpenGLIndexBuffer::GetCount() const { return m_Count; }

void OpenGLIndexBuffer::Bind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID); }

void OpenGLIndexBuffer::Unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

/////////////////////////////////////////////////////////////////////////////
// OpenGLUniformBuffer //////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLUniformBuffer::OpenGLUniformBuffer(const void* data, uint32 size) {
    glCreateBuffers(1, &m_RendererID);
    glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
    glBufferData(GL_UNIFORM_BUFFER, size, data, GL_STATIC_DRAW);
    //glNamedBufferData(m_RendererID, size, data, GL_STATIC_DRAW);
}

OpenGLUniformBuffer::~OpenGLUniformBuffer() { glDeleteBuffers(1, &m_RendererID); }

void OpenGLUniformBuffer::SetData(const void* data, uint32 size, uint32 offset) {
    glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
    glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
    //glNamedBufferSubData(m_RendererID, offset, size, data);
}

void OpenGLUniformBuffer::Bind(uint32 bindingPoint) const {
    glBindBuffer(GL_UNIFORM_BUFFER, m_RendererID);
    glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_RendererID);
}

void OpenGLUniformBuffer::Unbind() const { glBindBuffer(GL_UNIFORM_BUFFER, 0); }
} // namespace iGe
