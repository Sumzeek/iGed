module iGe.Renderer;
import :RendererAPI;
import :OpenGLRendererAPI;

import iGe.Log;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// RendererAPI ////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
RendererAPI::API RendererAPI::s_API = RendererAPI::API::OpenGL;

Scope<RendererAPI> RendererAPI::Create() {
    switch (s_API) {
        case RendererAPI::API::None:
            Internal::Assert(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateScope<OpenGLRendererAPI>();
        case RendererAPI::API::Vulkan:
            Internal::Assert(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    Internal::Assert(false, "Unknown RendererAPI!");
    return nullptr;
}
} // namespace iGe
