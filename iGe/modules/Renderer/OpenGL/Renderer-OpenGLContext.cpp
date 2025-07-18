module;
#include "iGeMacro.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

module iGe.Renderer;
import :OpenGLContext;

import iGe.Log;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// OpenGLContext ////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OpenGLContext::OpenGLContext(GLFWwindow* windowHandle) : m_WindowHandle{windowHandle} {
    IGE_CORE_ASSERT(windowHandle, "Window handle is nullptr!");
}

void OpenGLContext::Init() {
    glfwMakeContextCurrent(m_WindowHandle);
    int status = gladLoadGL((GLADloadfunc) glfwGetProcAddress);
    IGE_CORE_ASSERT(status, "Failed to initialize Glad!");

    IGE_CORE_INFO("OpenGL Info:");
    IGE_CORE_INFO("    Vendor: {0}", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
    IGE_CORE_INFO("    Renderer: {0}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
    IGE_CORE_INFO("    Version: {0}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
}

void OpenGLContext::SwapBuffers() { glfwSwapBuffers(m_WindowHandle); }

} // namespace iGe
