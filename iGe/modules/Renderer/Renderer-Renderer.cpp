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
Ref<Buffer> Renderer::s_SceneDataUniform = nullptr;

void Renderer::Init() {
    RenderCommand::Init();

    // SceneData + Transform
    s_SceneDataUniform = Buffer::Create(nullptr, sizeof(SceneData) + sizeof(glm::mat4));
}

void Renderer::Shutdown() {}

void Renderer::OnWindowResize(uint32_t width, uint32_t height) { RenderCommand::SetViewport(0, 0, width, height); }

void Renderer::BeginScene(Camera& camera) {
    s_SceneData->ViewMatrix = camera.GetViewMatrix();
    s_SceneData->ProjectionMatrix = camera.GetProjectionMatrix();
    s_SceneData->ViewProjectionMatrix = camera.GetViewProjectionMatrix();
}

void Renderer::EndScene() {}

void Renderer::SubmitTris(const Ref<GraphicsShader>& shader, const Ref<VertexArray>& vertexArray,
                          const glm::mat4& transform) {
    shader->Bind();

    s_SceneDataUniform->Bind(0, BufferType::Uniform);
    s_SceneDataUniform->SetData(s_SceneData.get(), sizeof(SceneData));
    s_SceneDataUniform->SetData(&transform, sizeof(transform), sizeof(SceneData));

    vertexArray->Bind();
    RenderCommand::DrawTriIndexed(vertexArray);
}

void Renderer::SubmitQuads(const Ref<GraphicsShader>& shader, const Ref<VertexArray>& vertexArray,
                           const glm::mat4& transform) {
    shader->Bind();

    s_SceneDataUniform->Bind(0, BufferType::Uniform);
    s_SceneDataUniform->SetData(s_SceneData.get(), sizeof(SceneData));
    s_SceneDataUniform->SetData(&transform, sizeof(transform), sizeof(SceneData));

    vertexArray->Bind();
    RenderCommand::DrawQuadIndexed(vertexArray);
}

void Renderer::SubmitPatches(const Ref<GraphicsShader>& shader, const Ref<VertexArray>& vertexArray,
                             uint32_t patchVertexCount, const glm::mat4& transform) {
    shader->Bind();

    s_SceneDataUniform->Bind(0, BufferType::Uniform);
    s_SceneDataUniform->SetData(s_SceneData.get(), sizeof(SceneData));
    s_SceneDataUniform->SetData(&transform, sizeof(transform), sizeof(SceneData));

    vertexArray->Bind();
    RenderCommand::DrawPatches(vertexArray, patchVertexCount);
}

void Renderer::Dispatch(const Ref<ComputeShader>& shader, const glm::uvec3 groupSize, const glm::mat4& transform) {
    shader->Bind();

    s_SceneDataUniform->Bind(0, BufferType::Uniform);
    s_SceneDataUniform->SetData(s_SceneData.get(), sizeof(SceneData));
    s_SceneDataUniform->SetData(&transform, sizeof(transform), sizeof(SceneData));

    shader->Dispatch(groupSize.x, groupSize.y, groupSize.z);
}

RendererAPI::API Renderer::GetAPI() { return RendererAPI::GetAPI(); }

} // namespace iGe
