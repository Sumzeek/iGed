module;
#include "Common/Core.h"
#include "Common/iGepch.h"
#include <GLFW/glfw3.h>

export module iGe.Window:Windows;
import :Base;
import iGe.Event;
import iGe.Log;

namespace iGe
{

export class IGE_API WindowsWindow : public Window {
public:
    WindowsWindow(const WindowProps& props);
    virtual ~WindowsWindow();

    void OnUpdate() override;

    virtual unsigned int GetWidth() const override;
    virtual unsigned int GetHeight() const override;

    // Window attributes
    void SetEventCallback(const EventCallbackFn& callback) override;
    void SetVSync(bool enable) override;
    bool IsVSync() const override;

private:
    virtual void Init(const WindowProps& props);
    virtual void ShutDown();

    struct WindowData {
        std::string Title;
        unsigned int Width;
        unsigned int Height;
        bool VSync;

        EventCallbackFn EventCallback;
    };

    GLFWwindow* m_Window;
    WindowData m_Data;
};

// ----------------- WindowsWindow::Implementation -----------------
static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error_code, const char* description) {
    Log::CoreError(std::format("GLFW Error ({0}): {1}", error_code, description));
}

WindowsWindow::WindowsWindow(const iGe::WindowProps& props) { Init(props); }

WindowsWindow::~WindowsWindow() { ShutDown(); }

Window* Window::Create(const iGe::WindowProps& props) { return new WindowsWindow{props}; }

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

    Log::CoreInfo(std::format("Createing window {0} ({1}, {2})", m_Data.Title, m_Data.Width, m_Data.Height));

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

        WindowResizeEvent event{static_cast<unsigned int>(width), static_cast<unsigned int>(height)};
        data->EventCallback(event);
    });

    glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window) {
        auto data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));
        WindowCloseEvent event{};
        data->EventCallback(event);
    });

    glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        auto data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

        switch (action) {
            case GLFW_PRESS: {
                KeyPressedEvent event{key, 0};
                data->EventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                KeyReleasedEvent event{key};
                data->EventCallback(event);
                break;
            }
            case GLFW_REPEAT: {
                KeyPressedEvent event{key, 1};
                data->EventCallback(event);
                break;
            }
        }
    });

    glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
        auto data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

        switch (action) {
            case GLFW_PRESS: {
                MouseButtonPressedEvent event{button};
                data->EventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                MouseButtonReleasedEvent event{button};
                data->EventCallback(event);
                break;
            }
        }
    });

    glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xOffset, double yOffset) {
        auto data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

        MouseScrolledEvent event{static_cast<float>(xOffset), static_cast<float>(yOffset)};
        data->EventCallback(event);
    });

    glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xPos, double yPos) {
        auto data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

        MouseMoveEvent event{static_cast<float>(xPos), static_cast<float>(yPos)};
        data->EventCallback(event);
    });
}

void WindowsWindow::ShutDown() {}

} // namespace iGe
