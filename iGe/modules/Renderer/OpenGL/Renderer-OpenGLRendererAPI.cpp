module;
#include "iGeMacro.h"

#include <glad/gl.h>

module iGe.Renderer;
import :OpenGLRendererAPI;
import iGe.Log;

namespace iGe
{
static void OpenGLMessageCallback(unsigned source, unsigned type, unsigned id, unsigned severity, int length,
                                  const char* message, const void* userParam) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            IGE_CORE_CRITICAL("OpenGL message: {}", message);
            return;
        case GL_DEBUG_SEVERITY_MEDIUM:
            IGE_CORE_ERROR("OpenGL message: {}", message);
            return;
        case GL_DEBUG_SEVERITY_LOW:
            IGE_CORE_WARN("OpenGL message: {}", message);
            return;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            IGE_CORE_TRACE("OpenGL message: {}", message);
            return;
    }

    IGE_CORE_ASSERT(false, "Unknown severity level!");
}

/////////////////////////////////////////////////////////////////////////////
// OpenGLRendererAPI ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void OpenGLRendererAPI::Init() {
#ifdef IGE_DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(OpenGLMessageCallback, nullptr);

    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
#endif

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);
}

void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    glViewport(x, y, width, height);
}

void OpenGLRendererAPI::SetClearColor(const glm::vec4& color) { glClearColor(color.r, color.g, color.b, color.a); }

void OpenGLRendererAPI::Clear() { glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); }

void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount) {
    vertexArray->Bind();
    uint32_t count = indexCount ? indexCount : vertexArray->GetIndexBuffer()->GetCount();
    glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
}

void OpenGLRendererAPI::DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) {
    vertexArray->Bind();
    glDrawArrays(GL_LINES, 0, vertexCount);
}

void OpenGLRendererAPI::SetLineWidth(float width) { glLineWidth(width); }

} // namespace iGe
