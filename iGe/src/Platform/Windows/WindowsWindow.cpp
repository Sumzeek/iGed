#include "Platform/Windows/WindowsWindow.h"
#include "iGepch.h"

#include "iGe/Events/ApplicationEvent.h"
#include "iGe/Events/KeyEvent.h"
#include "iGe/Events/MouseEvent.h"

namespace iGe
{

static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error_code, const char* description) {
    IGE_CORE_ERROR("GLFW Error ({0}): {1}", error_code, description);
}

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
        glfwSetErrorCallback(GLFWErrorCallback);

        s_GLFWInitialized = true;
    }

    m_Window = glfwCreateWindow((int) m_Data.Width, (int) m_Data.Height, m_Data.Title.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(m_Window);
    glfwSetWindowUserPointer(m_Window, &m_Data);
    SetVSync(true);

    // Set GLFW callbacks
    glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height) {
        auto data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));
        data->Width = width;
        data->Height = height;

        WindowResizeEvent event(width, height);
        data->EventCallback(event);
    });

    glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
        auto data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));
        WindowCloseEvent event;
        data->EventCallback(event);
    });

    glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

        switch (action) {
            case GLFW_PRESS: {
                KeyPressedEvent event(key, 0);
                data->EventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                KeyReleasedEvent event(key);
                data->EventCallback(event);
                break;
            }
            case GLFW_REPEAT: {
                KeyPressedEvent event(key, 1);
                data->EventCallback(event);
                break;
            }
        }
    });

    glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
        auto data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

        switch (action) {
            case GLFW_PRESS: {
                MouseButtonPressedEvent event(button);
                data->EventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                MouseButtonReleasedEvent event(button);
                data->EventCallback(event);
                break;
            }
        }
    });

    glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
        auto data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

        MouseScrolledEvent event((float) xOffset, (float) yOffset);
        data->EventCallback(event);
    });

    glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
        auto data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

        MouseMoveEvent event((float) xPos, (float) yPos);
        data->EventCallback(event);
    });
}

void WindowsWindow::ShutDown() {}

} // namespace iGe
