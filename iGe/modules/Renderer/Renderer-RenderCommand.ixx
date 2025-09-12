module;
#include "iGeMacro.h"

export module iGe.Renderer:RenderCommand;
import :RendererAPI;
import iGe.Common;
import glm;

namespace iGe
{
export class IGE_API RenderCommand {
public:
    inline static void Init() { s_RendererAPI->Init(); }

    inline static void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height) {
        s_RendererAPI->SetViewport(x, y, width, height);
    }

    inline static void SetClearColor(const glm::vec4& color) { s_RendererAPI->SetClearColor(color); }

    inline static void Clear() { s_RendererAPI->Clear(); }

    inline static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32 indexCount = 0) {
        s_RendererAPI->DrawIndexed(vertexArray, indexCount);
    }

    inline static void DrawLines(const Ref<VertexArray>& vertexArray, uint32 vertexCount) {
        s_RendererAPI->DrawLines(vertexArray, vertexCount);
    }

    inline static void SetLineWidth(float width) { s_RendererAPI->SetLineWidth(width); }

private:
    static Scope<RendererAPI> s_RendererAPI;
};
} // namespace iGe
