module;
#include "iGeMacro.h"

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

RendererAPI::API RendererAPI::GetAPI() { return s_API; }

Scope<RendererAPI> RendererAPI::Create() {
    switch (s_API) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateScope<OpenGLRendererAPI>();
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

} // namespace iGe