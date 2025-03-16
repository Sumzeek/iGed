#include "iGe/Application.h"
#include "iGepch.h"

#include <GLFW/glfw3.h>

namespace iGe
{

Application::Application() { m_Window = std::unique_ptr<Window>(Window::Create()); }

Application::~Application() {}

void Application::Run() {
    while (m_Running) {
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        m_Window->OnUpdate();
    }
}

} // namespace iGe
