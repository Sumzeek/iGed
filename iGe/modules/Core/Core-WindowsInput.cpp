module;
#include "iGeMacro.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

module iGe.Core;
import :WindowsInput;
import std;

namespace iGe
{
static int iGeKeyToGlfwKey(iGeKey keycode) {
    switch (keycode) {
        // Mouse Buttons
        case iGeKey::MouseLeft:
            return GLFW_MOUSE_BUTTON_LEFT;
        case iGeKey::MouseRight:
            return GLFW_MOUSE_BUTTON_RIGHT;
        case iGeKey::MouseMiddle:
            return GLFW_MOUSE_BUTTON_MIDDLE;
        case iGeKey::MouseButton4:
            return GLFW_MOUSE_BUTTON_4;
        case iGeKey::MouseButton5:
            return GLFW_MOUSE_BUTTON_5;

        // Number Keys (Top row)
        case iGeKey::_0:
            return GLFW_KEY_0;
        case iGeKey::_1:
            return GLFW_KEY_1;
        case iGeKey::_2:
            return GLFW_KEY_2;
        case iGeKey::_3:
            return GLFW_KEY_3;
        case iGeKey::_4:
            return GLFW_KEY_4;
        case iGeKey::_5:
            return GLFW_KEY_5;
        case iGeKey::_6:
            return GLFW_KEY_6;
        case iGeKey::_7:
            return GLFW_KEY_7;
        case iGeKey::_8:
            return GLFW_KEY_8;
        case iGeKey::_9:
            return GLFW_KEY_9;

        // Alphabet Keys
        case iGeKey::A:
            return GLFW_KEY_A;
        case iGeKey::B:
            return GLFW_KEY_B;
        case iGeKey::C:
            return GLFW_KEY_C;
        case iGeKey::D:
            return GLFW_KEY_D;
        case iGeKey::E:
            return GLFW_KEY_E;
        case iGeKey::F:
            return GLFW_KEY_F;
        case iGeKey::G:
            return GLFW_KEY_G;
        case iGeKey::H:
            return GLFW_KEY_H;
        case iGeKey::I:
            return GLFW_KEY_I;
        case iGeKey::J:
            return GLFW_KEY_J;
        case iGeKey::K:
            return GLFW_KEY_K;
        case iGeKey::L:
            return GLFW_KEY_L;
        case iGeKey::M:
            return GLFW_KEY_M;
        case iGeKey::N:
            return GLFW_KEY_N;
        case iGeKey::O:
            return GLFW_KEY_O;
        case iGeKey::P:
            return GLFW_KEY_P;
        case iGeKey::Q:
            return GLFW_KEY_Q;
        case iGeKey::R:
            return GLFW_KEY_R;
        case iGeKey::S:
            return GLFW_KEY_S;
        case iGeKey::T:
            return GLFW_KEY_T;
        case iGeKey::U:
            return GLFW_KEY_U;
        case iGeKey::V:
            return GLFW_KEY_V;
        case iGeKey::W:
            return GLFW_KEY_W;
        case iGeKey::X:
            return GLFW_KEY_X;
        case iGeKey::Y:
            return GLFW_KEY_Y;
        case iGeKey::Z:
            return GLFW_KEY_Z;

        // Function Keys
        case iGeKey::F1:
            return GLFW_KEY_F1;
        case iGeKey::F2:
            return GLFW_KEY_F2;
        case iGeKey::F3:
            return GLFW_KEY_F3;
        case iGeKey::F4:
            return GLFW_KEY_F4;
        case iGeKey::F5:
            return GLFW_KEY_F5;
        case iGeKey::F6:
            return GLFW_KEY_F6;
        case iGeKey::F7:
            return GLFW_KEY_F7;
        case iGeKey::F8:
            return GLFW_KEY_F8;
        case iGeKey::F9:
            return GLFW_KEY_F9;
        case iGeKey::F10:
            return GLFW_KEY_F10;
        case iGeKey::F11:
            return GLFW_KEY_F11;
        case iGeKey::F12:
            return GLFW_KEY_F12;

        // Numpad Keys
        case iGeKey::Numpad0:
            return GLFW_KEY_KP_0;
        case iGeKey::Numpad1:
            return GLFW_KEY_KP_1;
        case iGeKey::Numpad2:
            return GLFW_KEY_KP_2;
        case iGeKey::Numpad3:
            return GLFW_KEY_KP_3;
        case iGeKey::Numpad4:
            return GLFW_KEY_KP_4;
        case iGeKey::Numpad5:
            return GLFW_KEY_KP_5;
        case iGeKey::Numpad6:
            return GLFW_KEY_KP_6;
        case iGeKey::Numpad7:
            return GLFW_KEY_KP_7;
        case iGeKey::Numpad8:
            return GLFW_KEY_KP_8;
        case iGeKey::Numpad9:
            return GLFW_KEY_KP_9;
        case iGeKey::NumpadAdd:
            return GLFW_KEY_KP_ADD;
        case iGeKey::NumpadSubtract:
            return GLFW_KEY_KP_SUBTRACT;
        case iGeKey::NumpadMultiply:
            return GLFW_KEY_KP_MULTIPLY;
        case iGeKey::NumpadDivide:
            return GLFW_KEY_KP_DIVIDE;
        case iGeKey::NumpadEnter:
            return GLFW_KEY_KP_ENTER;
        case iGeKey::NumpadDecimal:
            return GLFW_KEY_KP_DECIMAL;

        // Control Keys
        case iGeKey::Tab:
            return GLFW_KEY_TAB;
        case iGeKey::Enter:
            return GLFW_KEY_ENTER;
        case iGeKey::LeftShift:
            return GLFW_KEY_LEFT_SHIFT;
        case iGeKey::RightShift:
            return GLFW_KEY_RIGHT_SHIFT;
        case iGeKey::LeftControl:
            return GLFW_KEY_LEFT_CONTROL;
        case iGeKey::RightControl:
            return GLFW_KEY_RIGHT_CONTROL;
        case iGeKey::LeftAlt:
            return GLFW_KEY_LEFT_ALT;
        case iGeKey::RightAlt:
            return GLFW_KEY_RIGHT_ALT;
        case iGeKey::LeftSuper:
            return GLFW_KEY_LEFT_SUPER;
        case iGeKey::RightSuper:
            return GLFW_KEY_RIGHT_SUPER;
        case iGeKey::Space:
            return GLFW_KEY_SPACE;
        case iGeKey::CapsLock:
            return GLFW_KEY_CAPS_LOCK;
        case iGeKey::Escape:
            return GLFW_KEY_ESCAPE;
        case iGeKey::Backspace:
            return GLFW_KEY_BACKSPACE;
        case iGeKey::PageUp:
            return GLFW_KEY_PAGE_UP;
        case iGeKey::PageDown:
            return GLFW_KEY_PAGE_DOWN;
        case iGeKey::Home:
            return GLFW_KEY_HOME;
        case iGeKey::End:
            return GLFW_KEY_END;
        case iGeKey::Insert:
            return GLFW_KEY_INSERT;
        case iGeKey::Delete:
            return GLFW_KEY_DELETE;
        case iGeKey::LeftArrow:
            return GLFW_KEY_LEFT;
        case iGeKey::UpArrow:
            return GLFW_KEY_UP;
        case iGeKey::RightArrow:
            return GLFW_KEY_RIGHT;
        case iGeKey::DownArrow:
            return GLFW_KEY_DOWN;
        case iGeKey::NumLock:
            return GLFW_KEY_NUM_LOCK;
        case iGeKey::ScrollLock:
            return GLFW_KEY_SCROLL_LOCK;

        // Additional Keyboard Keys
        case iGeKey::Apostrophe:
            return GLFW_KEY_APOSTROPHE;
        case iGeKey::Comma:
            return GLFW_KEY_COMMA;
        case iGeKey::Minus:
            return GLFW_KEY_MINUS;
        case iGeKey::Period:
            return GLFW_KEY_PERIOD;
        case iGeKey::Slash:
            return GLFW_KEY_SLASH;
        case iGeKey::Semicolon:
            return GLFW_KEY_SEMICOLON;
        case iGeKey::Equal:
            return GLFW_KEY_EQUAL;
        case iGeKey::LeftBracket:
            return GLFW_KEY_LEFT_BRACKET;
        case iGeKey::Backslash:
            return GLFW_KEY_BACKSLASH;
        case iGeKey::RightBracket:
            return GLFW_KEY_RIGHT_BRACKET;
        case iGeKey::GraveAccent:
            return GLFW_KEY_GRAVE_ACCENT;

        default:
            IGE_CORE_WARN("iGeKey {} is not mapped in GLFW!", keycode);
            return -1; // Return invalid GLFW key
    }
}

/////////////////////////////////////////////////////////////////////////////
// WindowsInput /////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
bool WindowsInput::IsKeyPressedImpl(iGeKey keycode) {
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto state = glfwGetKey(window, iGeKeyToGlfwKey(keycode));
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool WindowsInput::IsMouseButtonPressedImpl(iGeKey button) {
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto state = glfwGetMouseButton(window, iGeKeyToGlfwKey(button));
    return state == GLFW_PRESS;
}

std::pair<float, float> WindowsInput::GetMousePositionImpl() {
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    return {(float) xpos, (float) ypos};
}

float WindowsInput::GetMouseXImpl() {
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto [x, y] = GetMousePosition();
    return x;
}

float WindowsInput::GetMouseYImpl() {
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto [x, y] = GetMousePosition();
    return y;
}

} // namespace iGe