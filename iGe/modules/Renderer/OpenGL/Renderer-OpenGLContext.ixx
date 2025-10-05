module;
#include "iGeMacro.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

export module iGe.Renderer:OpenGLContext;
import :GraphicsContext;

namespace iGe
{
export class IGE_API OpenGLContext : public GraphicsContext {
public:
    OpenGLContext(GLFWwindow* windowHandle);

    virtual void Init() const override;
    virtual void SwapBuffers() const override;

private:
    GLFWwindow* m_WindowHandle;
};
} // namespace iGe
