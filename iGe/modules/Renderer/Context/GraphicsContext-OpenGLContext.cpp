module;
#include "iGeMacro.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

module iGe.GraphicsContext;

namespace iGe
{
// ---------------------------------- OpenGLContext::Implementation ----------------------------------
OpenGLContext::OpenGLContext(GLFWwindow* windowHandle) : m_WindowHandle{windowHandle} {
    IGE_CORE_ASSERT(windowHandle, "Window handle is nullptr!");
}

void OpenGLContext::Init() {
    glfwMakeContextCurrent(m_WindowHandle);
    int status = gladLoadGL((GLADloadfunc) glfwGetProcAddress);
    IGE_CORE_ASSERT(status, "Failed to initialize Glad!");
}

void OpenGLContext::SwapBuffers() { glfwSwapBuffers(m_WindowHandle); }

} // namespace iGe
