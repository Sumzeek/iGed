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
    static void Init() { s_RendererAPI->Init(); }

    static void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height) {
        s_RendererAPI->SetViewport(x, y, width, height);
    }

    static void SetClearColor(const glm::vec4& color) { s_RendererAPI->SetClearColor(color); }

    static void Clear() { s_RendererAPI->Clear(); }

    static void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32 indexCount = 0) {
        s_RendererAPI->DrawIndexed(vertexArray, indexCount);
    }

    static void DrawLines(const Ref<VertexArray>& vertexArray, uint32 vertexCount) {
        s_RendererAPI->DrawLines(vertexArray, vertexCount);
    }

    static void SetLineWidth(float32 width) { s_RendererAPI->SetLineWidth(width); }

private:
    static Scope<RendererAPI> s_RendererAPI;
};
} // namespace iGe
