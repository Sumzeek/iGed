module;
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

module iGe.Renderer;
import :OpenGLContext;
import iGe.Common;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// OpenGLContext ////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLContext::OpenGLContext(GLFWwindow* windowHandle) : m_WindowHandle{windowHandle} {
    Internal::Assert(windowHandle, "Window handle is nullptr!");
}

void OpenGLContext::Init() {
    glfwMakeContextCurrent(m_WindowHandle);
    int status = gladLoadGL((GLADloadfunc) glfwGetProcAddress);
    Internal::Assert(status, "Failed to initialize Glad!");

    Internal::LogInfo("OpenGL Info:");
    Internal::LogInfo("    Vendor: {0}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    Internal::LogInfo("    Renderer: {0}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    Internal::LogInfo("    Version: {0}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
}

void OpenGLContext::SwapBuffers() { glfwSwapBuffers(m_WindowHandle); }
} // namespace iGe
