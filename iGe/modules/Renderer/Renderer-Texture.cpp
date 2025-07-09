module;
#include "iGeMacro.h"

module iGe.Renderer;
import :Texture;
import :Renderer;
import :OpenGLTexture;

import iGe.Log;

namespace iGe
{

/////////////////////////////////////////////////////////////////////////////
// Texture2D ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLTexture2D>(specification);
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

Ref<Texture2D> Texture2D::Create(const std::string& path) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLTexture2D>(path);
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

} // namespace iGe