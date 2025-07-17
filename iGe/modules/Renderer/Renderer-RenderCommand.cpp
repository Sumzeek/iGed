module;
#include "iGeMacro.h"

module iGe.Renderer;
import :RenderCommand;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// RenderCommand ////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Scope<RendererAPI> RenderCommand::s_RendererAPI = RendererAPI::Create();

void RenderCommand::Init() { s_RendererAPI->Init(); }

void RenderCommand::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    s_RendererAPI->SetViewport(x, y, width, height);
}

void RenderCommand::SetClearColor(const glm::vec4& color) { s_RendererAPI->SetClearColor(color); }

void RenderCommand::Clear() { s_RendererAPI->Clear(); }

void RenderCommand::DrawIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount) {
    s_RendererAPI->DrawIndexed(vertexArray, indexCount);
}

void RenderCommand::DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) {
    s_RendererAPI->DrawLines(vertexArray, vertexCount);
}

void RenderCommand::DrawPatches(const Ref<VertexArray>& vertexArray, uint32_t patchVertexCount, uint32_t indexCount) {
    s_RendererAPI->DrawPatches(vertexArray, patchVertexCount, indexCount);
}


void RenderCommand::SetLineWidth(float width) { s_RendererAPI->SetLineWidth(width); }


} // namespace iGe