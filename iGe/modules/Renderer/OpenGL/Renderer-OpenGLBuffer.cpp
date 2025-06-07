module;
#include "iGeMacro.h"

#include <glad/gl.h>

module iGe.Renderer;
import :OpenGLBuffer;
import iGe.Log;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// OpenGLVertexBuffer ///////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLVertexBuffer::OpenGLVertexBuffer(float* vertices, uint32_t size) {
    glCreateBuffers(1, &m_RendererID);

    glNamedBufferData(m_RendererID, size, vertices, GL_STATIC_DRAW);
    //glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    //glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

OpenGLVertexBuffer::~OpenGLVertexBuffer() { glDeleteBuffers(1, &m_RendererID); }

void OpenGLVertexBuffer::SetLayout(const BufferLayout& layout) { m_Layout = layout; }

const BufferLayout& OpenGLVertexBuffer::GetLayout() const { return m_Layout; }

void OpenGLVertexBuffer::Bind() const { glBindBuffer(GL_ARRAY_BUFFER, m_RendererID); }

void OpenGLVertexBuffer::Unbind() const { glBindBuffer(GL_ARRAY_BUFFER, 0); }

/////////////////////////////////////////////////////////////////////////////
// OpenGLVIndexBuffer ///////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLIndexBuffer::OpenGLIndexBuffer(uint32_t* indices, uint32_t count) : m_Count{count} {
    glCreateBuffers(1, &m_RendererID);

    glNamedBufferData(m_RendererID, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    // GL_ELEMENT_ARRAY_BUFFER is not valid without an actively bound VAO
    // Binding with GL_ARRAY_BUFFER allows the data to be loaded regardless of VAO state.
    //glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);;
    //glBufferData(GL_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
}

OpenGLIndexBuffer::~OpenGLIndexBuffer() { glDeleteBuffers(1, &m_RendererID); }

uint32_t OpenGLIndexBuffer::GetCount() const { return m_Count; }

void OpenGLIndexBuffer::Bind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID); }

void OpenGLIndexBuffer::Unbind() const { glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); }

/////////////////////////////////////////////////////////////////////////////
// OpenGLBuffer /////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static GLenum ToGLBufferTarget(BufferType type) {
    switch (type) {
        case BufferType::Uniform:
            return GL_UNIFORM_BUFFER;
        case BufferType::Storage:
            return GL_SHADER_STORAGE_BUFFER;
        default:
            IGE_CORE_ERROR("Unknown BufferType in ToGLBufferTarget");
            return 0;
    }
}

OpenGLBuffer::OpenGLBuffer(const void* data, uint32_t size) {
    glCreateBuffers(1, &m_RendererID);
    // TODO: usage hint
    glNamedBufferData(m_RendererID, size, data, GL_DYNAMIC_DRAW);
}

OpenGLBuffer::~OpenGLBuffer() { glDeleteBuffers(1, &m_RendererID); }

void OpenGLBuffer::SetData(const void* data, uint32_t size, uint32_t offset) {
    glNamedBufferSubData(m_RendererID, offset, size, data);
}

void OpenGLBuffer::GetData(void* data, uint32_t size, uint32_t offset) {
    glGetNamedBufferSubData(m_RendererID, offset, size, data);
}

void OpenGLBuffer::Bind(uint32_t slot, BufferType type) {
    m_Target = ToGLBufferTarget(type);
    if (m_Target == 0) { return; }

    glBindBuffer(m_Target, m_RendererID);
    glBindBufferBase(m_Target, slot, m_RendererID);
}

void OpenGLBuffer::Unbind() const { glBindBuffer(m_Target, 0); }

} // namespace iGe
