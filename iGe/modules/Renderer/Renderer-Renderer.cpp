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
Ref<UniformBuffer> Renderer::s_SceneDataUniform = nullptr;

void Renderer::Init() {
    RenderCommand::Init();

    // SceneData + Transform
    s_SceneDataUniform = UniformBuffer::Create(nullptr, sizeof(SceneData) + sizeof(glm::mat4));
}

void Renderer::Shutdown() {}

void Renderer::OnWindowResize(uint32_t width, uint32_t height) { RenderCommand::SetViewport(0, 0, width, height); }

void Renderer::BeginScene(OrthographicCamera& camera) {
    s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
}

void Renderer::EndScene() {}

void Renderer::Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const glm::mat4& transform) {
    shader->Bind();

    s_SceneDataUniform->Bind(0);
    s_SceneDataUniform->SetData(s_SceneData.get(), sizeof(SceneData));
    s_SceneDataUniform->SetData(&transform, sizeof(transform), sizeof(SceneData));

    vertexArray->Bind();
    RenderCommand::DrawIndexed(vertexArray);
}

RendererAPI::API Renderer::GetAPI() { return RendererAPI::GetAPI(); }

} // namespace iGe