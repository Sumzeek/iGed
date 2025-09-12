module iGe.Renderer;
import :VertexArray;
import :Renderer;
import :OpenGLVertexArray;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// VertexArray //////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Ref<VertexArray> VertexArray::Create() {
    switch (Renderer::GetAPI()) {
        case RendererAPI::API::None:
            Internal::Assert(false, "RendererAPI::None is currently not supported!");
            return nullptr;
        case RendererAPI::API::OpenGL:
            return CreateRef<OpenGLVertexArray>();
            return nullptr;
        case RendererAPI::API::Vulkan:
            Internal::Assert(false, "RendererAPI::Vulkan is currently not supported!");
            return nullptr;
    }

    Internal::Assert(false, "Unknown RendererAPI!");
    return nullptr;
}
} // namespace iGe
