module;
#include "iGeMacro.h"

export module iGe.Renderer:Renderer;
import :RendererAPI;
import :OrthographicCamera;
import :Shader;
import iGe.SmartPointer;

namespace iGe
{

export class IGE_API Renderer {
public:
    static void Init();
    static void Shutdown();

    static void OnWindowResize(uint32_t width, uint32_t height);

    static void BeginScene(OrthographicCamera& camera);
    static void EndScene();

    static void Submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray,
                       const glm::mat4& transform = glm::mat4(1.0f));

    static RendererAPI::API GetAPI();

private:
    struct SceneData {
        glm::mat4 ViewProjectionMatrix;
    };
    static Scope<SceneData> s_SceneData;
    static Ref<UniformBuffer> s_SceneDataUniform;
};

} // namespace iGe
