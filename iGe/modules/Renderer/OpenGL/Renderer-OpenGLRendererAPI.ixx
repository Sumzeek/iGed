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
    virtual void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height) override;

    virtual void SetClearColor(const glm::vec4& color) override;
    virtual void Clear() override;

    virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, uint32 indexCount = 0) override;
    virtual void DrawLines(const Ref<VertexArray>& vertexArray, uint32 vertexCount) override;

    virtual void SetLineWidth(float width) override;
};
} // namespace iGe
