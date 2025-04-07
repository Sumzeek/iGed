module;
#include "iGeMacro.h"
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

module iGe.Window;
import :WindowsWindow;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// WindowsWindow ////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error_code, const char* description) {
    IGE_CORE_ERROR("GLFW Error ({0}): {1}", error_code, description);
}

iGeKey GlfwKeyToiGeKey(int keycode);

WindowsWindow::WindowsWindow(const iGe::WindowProps& props) { Init(props); }

WindowsWindow::~WindowsWindow() { ShutDown(); }

void WindowsWindow::OnUpdate() {
    glfwPollEvents();
    m_Context->SwapBuffers();
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

void* WindowsWindow::GetNativeWindow() const { return m_Window; }

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

    m_Context = GraphicsContext::Create(m_Window);
    m_Context->Init();

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

        auto keycode = GlfwKeyToiGeKey(key);
        switch (action) {
            case GLFW_PRESS: {
                KeyPressedEvent event{keycode, 0};
                data->EventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                KeyReleasedEvent event{keycode};
                data->EventCallback(event);
                break;
            }
            case GLFW_REPEAT: {
                KeyPressedEvent event{keycode, 1};
                data->EventCallback(event);
                break;
            }
        }
    });

    glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int codepoint) {
        auto data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

        KeyTypedEvent event{codepoint};
        data->EventCallback(event);
    });

    glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods) {
        auto data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));

        auto keycode = GlfwKeyToiGeKey(button);
        switch (action) {
            case GLFW_PRESS: {
                MouseButtonPressedEvent event{keycode};
                data->EventCallback(event);
                break;
            }
            case GLFW_RELEASE: {
                MouseButtonReleasedEvent event{keycode};
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

iGeKey GlfwKeyToiGeKey(int keycode) {
    switch (keycode) {
        // Mouse Buttons
        case GLFW_MOUSE_BUTTON_LEFT:
            return iGeKey::MouseLeft;
        case GLFW_MOUSE_BUTTON_RIGHT:
            return iGeKey::MouseRight;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            return iGeKey::MouseMiddle;
        case GLFW_MOUSE_BUTTON_4:
            return iGeKey::MouseButton4;
        case GLFW_MOUSE_BUTTON_5:
            return iGeKey::MouseButton5;

        // Number keys (top row)
        case GLFW_KEY_0:
            return iGeKey::_0;
        case GLFW_KEY_1:
            return iGeKey::_1;
        case GLFW_KEY_2:
            return iGeKey::_2;
        case GLFW_KEY_3:
            return iGeKey::_3;
        case GLFW_KEY_4:
            return iGeKey::_4;
        case GLFW_KEY_5:
            return iGeKey::_5;
        case GLFW_KEY_6:
            return iGeKey::_6;
        case GLFW_KEY_7:
            return iGeKey::_7;
        case GLFW_KEY_8:
            return iGeKey::_8;
        case GLFW_KEY_9:
            return iGeKey::_9;

        // Alphabet keys
        case GLFW_KEY_A:
            return iGeKey::A;
        case GLFW_KEY_B:
            return iGeKey::B;
        case GLFW_KEY_C:
            return iGeKey::C;
        case GLFW_KEY_D:
            return iGeKey::D;
        case GLFW_KEY_E:
            return iGeKey::E;
        case GLFW_KEY_F:
            return iGeKey::F;
        case GLFW_KEY_G:
            return iGeKey::G;
        case GLFW_KEY_H:
            return iGeKey::H;
        case GLFW_KEY_I:
            return iGeKey::I;
        case GLFW_KEY_J:
            return iGeKey::J;
        case GLFW_KEY_K:
            return iGeKey::K;
        case GLFW_KEY_L:
            return iGeKey::L;
        case GLFW_KEY_M:
            return iGeKey::M;
        case GLFW_KEY_N:
            return iGeKey::N;
        case GLFW_KEY_O:
            return iGeKey::O;
        case GLFW_KEY_P:
            return iGeKey::P;
        case GLFW_KEY_Q:
            return iGeKey::Q;
        case GLFW_KEY_R:
            return iGeKey::R;
        case GLFW_KEY_S:
            return iGeKey::S;
        case GLFW_KEY_T:
            return iGeKey::T;
        case GLFW_KEY_U:
            return iGeKey::U;
        case GLFW_KEY_V:
            return iGeKey::V;
        case GLFW_KEY_W:
            return iGeKey::W;
        case GLFW_KEY_X:
            return iGeKey::X;
        case GLFW_KEY_Y:
            return iGeKey::Y;
        case GLFW_KEY_Z:
            return iGeKey::Z;

        // Function keys
        case GLFW_KEY_F1:
            return iGeKey::F1;
        case GLFW_KEY_F2:
            return iGeKey::F2;
        case GLFW_KEY_F3:
            return iGeKey::F3;
        case GLFW_KEY_F4:
            return iGeKey::F4;
        case GLFW_KEY_F5:
            return iGeKey::F5;
        case GLFW_KEY_F6:
            return iGeKey::F6;
        case GLFW_KEY_F7:
            return iGeKey::F7;
        case GLFW_KEY_F8:
            return iGeKey::F8;
        case GLFW_KEY_F9:
            return iGeKey::F9;
        case GLFW_KEY_F10:
            return iGeKey::F10;
        case GLFW_KEY_F11:
            return iGeKey::F11;
        case GLFW_KEY_F12:
            return iGeKey::F12;

        // Numpad keys
        case GLFW_KEY_KP_0:
            return iGeKey::Numpad0;
        case GLFW_KEY_KP_1:
            return iGeKey::Numpad1;
        case GLFW_KEY_KP_2:
            return iGeKey::Numpad2;
        case GLFW_KEY_KP_3:
            return iGeKey::Numpad3;
        case GLFW_KEY_KP_4:
            return iGeKey::Numpad4;
        case GLFW_KEY_KP_5:
            return iGeKey::Numpad5;
        case GLFW_KEY_KP_6:
            return iGeKey::Numpad6;
        case GLFW_KEY_KP_7:
            return iGeKey::Numpad7;
        case GLFW_KEY_KP_8:
            return iGeKey::Numpad8;
        case GLFW_KEY_KP_9:
            return iGeKey::Numpad9;
        case GLFW_KEY_KP_ADD:
            return iGeKey::NumpadAdd;
        case GLFW_KEY_KP_SUBTRACT:
            return iGeKey::NumpadSubtract;
        case GLFW_KEY_KP_MULTIPLY:
            return iGeKey::NumpadMultiply;
        case GLFW_KEY_KP_DIVIDE:
            return iGeKey::NumpadDivide;
        case GLFW_KEY_KP_ENTER:
            return iGeKey::NumpadEnter;
        case GLFW_KEY_KP_DECIMAL:
            return iGeKey::NumpadDecimal;

        // Control Keys
        case GLFW_KEY_TAB:
            return iGeKey::Tab;
        case GLFW_KEY_ENTER:
            return iGeKey::Enter;
        case GLFW_KEY_LEFT_SHIFT:
            return iGeKey::LeftShift;
        case GLFW_KEY_RIGHT_SHIFT:
            return iGeKey::RightShift;
        case GLFW_KEY_LEFT_CONTROL:
            return iGeKey::LeftControl;
        case GLFW_KEY_RIGHT_CONTROL:
            return iGeKey::RightControl;
        case GLFW_KEY_LEFT_ALT:
            return iGeKey::LeftAlt;
        case GLFW_KEY_RIGHT_ALT:
            return iGeKey::RightAlt;
        case GLFW_KEY_LEFT_SUPER:
            return iGeKey::LeftSuper;
        case GLFW_KEY_RIGHT_SUPER:
            return iGeKey::RightSuper;
        case GLFW_KEY_SPACE:
            return iGeKey::Space;
        case GLFW_KEY_CAPS_LOCK:
            return iGeKey::CapsLock;
        case GLFW_KEY_ESCAPE:
            return iGeKey::Escape;
        case GLFW_KEY_BACKSPACE:
            return iGeKey::Backspace;
        case GLFW_KEY_PAGE_UP:
            return iGeKey::PageUp;
        case GLFW_KEY_PAGE_DOWN:
            return iGeKey::PageDown;
        case GLFW_KEY_HOME:
            return iGeKey::Home;
        case GLFW_KEY_END:
            return iGeKey::End;
        case GLFW_KEY_INSERT:
            return iGeKey::Insert;
        case GLFW_KEY_DELETE:
            return iGeKey::Delete;
        case GLFW_KEY_LEFT:
            return iGeKey::LeftArrow;
        case GLFW_KEY_UP:
            return iGeKey::UpArrow;
        case GLFW_KEY_RIGHT:
            return iGeKey::RightArrow;
        case GLFW_KEY_DOWN:
            return iGeKey::DownArrow;
        case GLFW_KEY_NUM_LOCK:
            return iGeKey::NumLock;
        case GLFW_KEY_SCROLL_LOCK:
            return iGeKey::ScrollLock;

        // Additional Keyboard Keys
        case GLFW_KEY_APOSTROPHE:
            return iGeKey::Apostrophe;
        case GLFW_KEY_COMMA:
            return iGeKey::Comma;
        case GLFW_KEY_MINUS:
            return iGeKey::Minus;
        case GLFW_KEY_PERIOD:
            return iGeKey::Period;
        case GLFW_KEY_SLASH:
            return iGeKey::Slash;
        case GLFW_KEY_SEMICOLON:
            return iGeKey::Semicolon;
        case GLFW_KEY_EQUAL:
            return iGeKey::Equal;
        case GLFW_KEY_LEFT_BRACKET:
            return iGeKey::LeftBracket;
        case GLFW_KEY_BACKSLASH:
            return iGeKey::Backslash;
        case GLFW_KEY_RIGHT_BRACKET:
            return iGeKey::RightBracket;
        case GLFW_KEY_GRAVE_ACCENT:
            return iGeKey::GraveAccent;

        default:
            IGE_CORE_WARN("Keycode {} in GLFW is not mapped in iGeKey!", keycode);
            return iGeKey::None; // Return None for any unrecognized keys
    }
}

} // namespace iGe