module;
#include "iGeMacro.h"

export module iGe.Renderer:Renderer;
import :RendererAPI;
import :OrthographicCamera;
import :Shader;
import iGe.Common;

namespace iGe
{
export class IGE_API Renderer {
public:
    static void Init();
    static void Shutdown();

    static void OnWindowResize(uint32 width, uint32 height);

    static void BeginScene(OrthographicCamera& camera);
    static void EndScene();

    static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray,
                       const glm::mat4& transform = glm::mat4(1.0f));

    static RendererAPI::API GetAPI() { return RendererAPI::GetAPI(); }

private:
    struct SceneData {
        glm::mat4 ViewProjectionMatrix;
    };
    static Scope<SceneData> s_SceneData;
    static Ref<UniformBuffer> s_SceneDataUniform;
};
} // namespace iGe
