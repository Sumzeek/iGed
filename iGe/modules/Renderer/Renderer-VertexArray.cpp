module;
#include "iGeMacro.h"

module iGe.Renderer;
import :VertexArray;
import :Renderer;
import :OpenGLVertexArray;

import iGe.Log;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// VertexArray //////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

Ref<VertexArray> VertexArray::Create() {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLVertexArray>();
            return nullptr;
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
};

} // namespace iGe