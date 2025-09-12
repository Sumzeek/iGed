module;
#include "iGeMacro.h"

export module iGe.Renderer:RendererAPI;
import :VertexArray;
import iGe.Common;
import glm;

namespace iGe
{
export class IGE_API RendererAPI {
public:
    enum class API : int { None = 0, OpenGL = 1, Vulkan = 2 };

    virtual ~RendererAPI() = default;

    virtual void Init() = 0;
    virtual void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height) = 0;

    virtual void SetClearColor(const glm::vec4& color) = 0;
    virtual void Clear() = 0;

    virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32 indexCount = 0) = 0;
    virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32 vertexCount) = 0;

    virtual void SetLineWidth(float width) = 0;

    static API GetAPI() { return s_API; }
    static Scope<RendererAPI> Create();

private:
    static API s_API;
};
} // namespace iGe
