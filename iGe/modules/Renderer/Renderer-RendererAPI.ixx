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

    virtual void Init() const = 0;
    virtual void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height) const = 0;

    virtual void SetClearColor(const glm::vec4& color) const = 0;
    virtual void Clear() const = 0;

    virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32 indexCount = 0) const = 0;
    virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32 vertexCount) const = 0;

    virtual void SetLineWidth(float width) const = 0;

    static API GetAPI() { return s_API; }
    static Scope<RendererAPI> Create();

private:
    static API s_API;
};
} // namespace iGe
