module;
#include "iGeMacro.h"

module iGe.Renderer;
import :Renderer;
import :RenderCommand;
import iGe.Log;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// Renderer /////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Scope<Renderer::SceneData> Renderer::s_SceneData = CreateScope<Renderer::SceneData>();

void Renderer::Init() { RenderCommand::Init(); }

void Renderer::Shutdown() {}

void Renderer::OnWindowResize(uint32_t width, uint32_t height) { RenderCommand::SetViewport(0, 0, width, height); }

void Renderer::BeginScene(OrthographicCamera& camera) {
    s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
}

void Renderer::EndScene() {}

void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform) {
    shader->Bind();
    shader->SetMat4("u_ViewProjection", s_SceneData->ViewProjectionMatrix);
    shader->SetMat4("u_Transform", transform);

    vertexArray->Bind();
    RenderCommand::DrawIndexed(vertexArray);
}

RendererAPI::API Renderer::GetAPI() { return RendererAPI::GetAPI(); }

} // namespace iGe