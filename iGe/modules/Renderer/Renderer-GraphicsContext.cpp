module;
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

module iGe.Renderer;
import :Renderer;
import :GraphicsContext;
import :OpenGLContext;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// GraphicsContext //////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Scope<GraphicsContext> GraphicsContext::Create(void* window) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            Internal::Assert(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));
        case RendererAPI::API::Vulkan:
            Internal::Assert(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    Internal::Assert(false, "Unknown RendererAPI!");
    return nullptr;
}
} // namespace iGe
