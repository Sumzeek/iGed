module;
#include "iGeMacro.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

export module iGe.Renderer:OpenGLRendererAPI;
import :RendererAPI;

namespace iGe
{

export class IGE_API OpenGLRendererAPI : public RendererAPI {
public:
    virtual void Init() override;
    virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override;

    virtual void SetClearColor(const glm::vec4& color) override;
    virtual void Clear() override;

    virtual void DrawTri(const Ref<VertexArray>& vertexArray, uint32_t triCount) override;
    virtual void DrawTriIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
    virtual void DrawQuad(const Ref<VertexArray>& vertexArray, uint32_t quadCount) override;
    virtual void DrawQuadIndexed(const Ref<VertexArray>& vertexArray, uint32_t indexCount = 0) override;
    virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32_t vertexCount) override;
    virtual void DrawPatches(const Ref<VertexArray>& vertexArray, uint32_t patchVertexCount = 3,
                             uint32_t indexCount = 0) override;

    virtual void SetLineWidth(float width) override;
};

} // namespace iGe
