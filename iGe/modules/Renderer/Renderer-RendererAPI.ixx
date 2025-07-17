module;
#include "iGeMacro.h"

export module iGe.Renderer:RendererAPI;
import :VertexArray;
import std;
import glm;

namespace iGe
{

export class IGE_API RendererAPI {
public:
    enum class API : int { None = 0, OpenGL = 1, Vulkan = 2 };

    virtual ~RendererAPI() = default;

    virtual void Init() = 0;
    virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;

    virtual void SetClearColor(const glm::vec4& color) = 0;
    virtual void Clear() = 0;

    virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) = 0;
    virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) = 0;
    virtual void DrawPatches(const Ref<VertexArray>& vertexArray, uint32_t patchVertexCount = 3,
                             uint32_t indexCount = 0) = 0;

    virtual void SetLineWidth(float width) = 0;

    static API GetAPI();
    static Scope<RendererAPI> Create();

private:
    static API s_API;
};

} // namespace iGe
