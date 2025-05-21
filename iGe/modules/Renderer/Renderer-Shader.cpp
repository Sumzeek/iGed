module;
#include "iGeMacro.h"

module iGe.Renderer;
import :Shader;
import :OpenGLShader;
import std;
import iGe.Log;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// GraphicsShader ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Ref<GraphicsShader> GraphicsShader::Create(const std::string& filepath) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLGraphicsShader>(filepath);
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

Ref<GraphicsShader> GraphicsShader::Create(const std::string& name, const std::string& vertexSrc,
                                           const std::string& fragmentSrc) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLGraphicsShader>(name, vertexSrc, fragmentSrc);
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

/////////////////////////////////////////////////////////////////////////////
// ComputeShader ////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Ref<ComputeShader> ComputeShader::Create(const std::string& filepath) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLComputeShader>(filepath);
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

Ref<ComputeShader> ComputeShader::Create(const std::string& name, const std::string& computeSrc) {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            IGE_CORE_ASSERT(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLComputeShader>(name, computeSrc);
        case RendererAPI::API::Vulkan:
            IGE_CORE_ASSERT(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    IGE_CORE_ASSERT(false, "Unknown RendererAPI!");
    return nullptr;
}

} // namespace iGe