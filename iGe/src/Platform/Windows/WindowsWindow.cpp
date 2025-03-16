#include "Platform/Windows/WindowsWindow.h"
#include "iGepch.h"

namespace iGe
{

static bool s_GLFWInitialized = false;

Window* Window::Create(const iGe::WindowProps& props) { return new WindowsWindow(props); }

WindowsWindow::WindowsWindow(const iGe::WindowProps& props) { Init(props); }

WindowsWindow::~WindowsWindow() { ShutDown(); }

void WindowsWindow::OnUpdate() {
    glfwPollEvents();
    glfwSwapBuffers(m_Window);
}

unsigned int WindowsWindow::GetWidth() const { return m_Data.Width; }

unsigned int WindowsWindow::GetHeight() const { return m_Data.Height; }

void WindowsWindow::SetEventCallback(const EventCallbackFn& callback) { m_Data.EventCallback = callback; }

void WindowsWindow::SetVSync(bool enable) {
    if (enable) {
        glfwSwapInterval(1);
    } else {
        glfwSwapInterval(0);
    }
    m_Data.VSync = enable;
}

bool WindowsWindow::IsVSync() const { return m_Data.VSync; }

void WindowsWindow::Init(const iGe::WindowProps& props) {
    m_Data.Title = props.Title;
    m_Data.Width = props.Width;
    m_Data.Height = props.Height;

    IGE_CORE_INFO("Createing window {0} ({1}, {2})", m_Data.Title, m_Data.Width, m_Data.Height);

    if (!s_GLFWInitialized) {
        // TODO: glfwTerminate on system shutdown
        int success = glfwInit();
        IGE_CORE_ASSERT(success, "Could not initialize GLFW!");

        s_GLFWInitialized = true;
    }

    m_Window = glfwCreateWindow((int) m_Data.Width, (int) m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(m_Window);
    glfwSetWindowUserPointer(m_Window, &m_Data);
    SetVSync(true);
}

void WindowsWindow::ShutDown() {}

} // namespace iGe
