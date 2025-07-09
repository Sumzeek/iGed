module;
#include "iGeMacro.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

module iGe.Renderer;
import :Renderer;
import :GraphicsContext;
import :OpenGLContext;

import iGe.Log;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// GraphicsContext //////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

Scope<GraphicsContext> GraphicsContext::Create(void* window) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateScope<OpenGLContext>(static_cast<GLFWwindow*>(window));
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

} // namespace iGe