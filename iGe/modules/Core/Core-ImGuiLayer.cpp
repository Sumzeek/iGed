module;
#include "iGeMacro.h"

// TEMPORARY
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

module iGe.Core;
import std;

namespace iGe
{
static ImGuiKey iGeKeyToImGuiKey(iGeKey keycode) {
    switch (keycode) {
        // Mouse Buttons
        case iGeKey::MouseLeft:
            return ImGuiKey_MouseLeft;
        case iGeKey::MouseRight:
            return ImGuiKey_MouseRight;
        case iGeKey::MouseMiddle:
            return ImGuiKey_MouseMiddle;
        case iGeKey::MouseButton4:
            return ImGuiKey_MouseX1;
        case iGeKey::MouseButton5:
            return ImGuiKey_MouseX2;

        // Number Keys (Top row)
        case iGeKey::_0:
            return ImGuiKey_0;
        case iGeKey::_1:
            return ImGuiKey_1;
        case iGeKey::_2:
            return ImGuiKey_2;
        case iGeKey::_3:
            return ImGuiKey_3;
        case iGeKey::_4:
            return ImGuiKey_4;
        case iGeKey::_5:
            return ImGuiKey_5;
        case iGeKey::_6:
            return ImGuiKey_6;
        case iGeKey::_7:
            return ImGuiKey_7;
        case iGeKey::_8:
            return ImGuiKey_8;
        case iGeKey::_9:
            return ImGuiKey_9;

        // Alphabet Keys
        case iGeKey::A:
            return ImGuiKey_A;
        case iGeKey::B:
            return ImGuiKey_B;
        case iGeKey::C:
            return ImGuiKey_C;
        case iGeKey::D:
            return ImGuiKey_D;
        case iGeKey::E:
            return ImGuiKey_E;
        case iGeKey::F:
            return ImGuiKey_F;
        case iGeKey::G:
            return ImGuiKey_G;
        case iGeKey::H:
            return ImGuiKey_H;
        case iGeKey::I:
            return ImGuiKey_I;
        case iGeKey::J:
            return ImGuiKey_J;
        case iGeKey::K:
            return ImGuiKey_K;
        case iGeKey::L:
            return ImGuiKey_L;
        case iGeKey::M:
            return ImGuiKey_M;
        case iGeKey::N:
            return ImGuiKey_N;
        case iGeKey::O:
            return ImGuiKey_O;
        case iGeKey::P:
            return ImGuiKey_P;
        case iGeKey::Q:
            return ImGuiKey_Q;
        case iGeKey::R:
            return ImGuiKey_R;
        case iGeKey::S:
            return ImGuiKey_S;
        case iGeKey::T:
            return ImGuiKey_T;
        case iGeKey::U:
            return ImGuiKey_U;
        case iGeKey::V:
            return ImGuiKey_V;
        case iGeKey::W:
            return ImGuiKey_W;
        case iGeKey::X:
            return ImGuiKey_X;
        case iGeKey::Y:
            return ImGuiKey_Y;
        case iGeKey::Z:
            return ImGuiKey_Z;

            // Function Keys
        case iGeKey::F1:
            return ImGuiKey_F1;
        case iGeKey::F2:
            return ImGuiKey_F2;
        case iGeKey::F3:
            return ImGuiKey_F3;
        case iGeKey::F4:
            return ImGuiKey_F4;
        case iGeKey::F5:
            return ImGuiKey_F5;
        case iGeKey::F6:
            return ImGuiKey_F6;
        case iGeKey::F7:
            return ImGuiKey_F7;
        case iGeKey::F8:
            return ImGuiKey_F8;
        case iGeKey::F9:
            return ImGuiKey_F9;
        case iGeKey::F10:
            return ImGuiKey_F10;
        case iGeKey::F11:
            return ImGuiKey_F11;
        case iGeKey::F12:
            return ImGuiKey_F12;

        // Numpad Keys
        case iGeKey::Numpad0:
            return ImGuiKey_Keypad0;
        case iGeKey::Numpad1:
            return ImGuiKey_Keypad1;
        case iGeKey::Numpad2:
            return ImGuiKey_Keypad2;
        case iGeKey::Numpad3:
            return ImGuiKey_Keypad3;
        case iGeKey::Numpad4:
            return ImGuiKey_Keypad4;
        case iGeKey::Numpad5:
            return ImGuiKey_Keypad5;
        case iGeKey::Numpad6:
            return ImGuiKey_Keypad6;
        case iGeKey::Numpad7:
            return ImGuiKey_Keypad7;
        case iGeKey::Numpad8:
            return ImGuiKey_Keypad8;
        case iGeKey::Numpad9:
            return ImGuiKey_Keypad9;
        case iGeKey::NumpadAdd:
            return ImGuiKey_KeypadAdd;
        case iGeKey::NumpadSubtract:
            return ImGuiKey_KeypadSubtract;
        case iGeKey::NumpadMultiply:
            return ImGuiKey_KeypadMultiply;
        case iGeKey::NumpadDivide:
            return ImGuiKey_KeypadDivide;
        case iGeKey::NumpadEnter:
            return ImGuiKey_KeypadEnter;
        case iGeKey::NumpadDecimal:
            return ImGuiKey_KeypadDecimal;

        // Control Keys
        case iGeKey::Tab:
            return ImGuiKey_Tab;
        case iGeKey::Enter:
            return ImGuiKey_Enter;
        case iGeKey::LeftShift:
            return ImGuiKey_LeftShift;
        case iGeKey::RightShift:
            return ImGuiKey_RightShift;
        case iGeKey::LeftControl:
            return ImGuiKey_LeftCtrl;
        case iGeKey::RightControl:
            return ImGuiKey_RightCtrl;
        case iGeKey::LeftAlt:
            return ImGuiKey_LeftAlt;
        case iGeKey::RightAlt:
            return ImGuiKey_RightAlt;
        case iGeKey::LeftSuper:
            return ImGuiKey_LeftSuper;
        case iGeKey::RightSuper:
            return ImGuiKey_RightSuper;
        case iGeKey::Space:
            return ImGuiKey_Space;
        case iGeKey::CapsLock:
            return ImGuiKey_CapsLock;
        case iGeKey::Escape:
            return ImGuiKey_Escape;
        case iGeKey::Backspace:
            return ImGuiKey_Backspace;
        case iGeKey::PageUp:
            return ImGuiKey_PageUp;
        case iGeKey::PageDown:
            return ImGuiKey_PageDown;
        case iGeKey::Home:
            return ImGuiKey_Home;
        case iGeKey::End:
            return ImGuiKey_End;
        case iGeKey::Insert:
            return ImGuiKey_Insert;
        case iGeKey::Delete:
            return ImGuiKey_Delete;
        case iGeKey::LeftArrow:
            return ImGuiKey_LeftArrow;
        case iGeKey::UpArrow:
            return ImGuiKey_UpArrow;
        case iGeKey::RightArrow:
            return ImGuiKey_RightArrow;
        case iGeKey::DownArrow:
            return ImGuiKey_DownArrow;
        case iGeKey::NumLock:
            return ImGuiKey_NumLock;
        case iGeKey::ScrollLock:
            return ImGuiKey_ScrollLock;

        // Additional Keyboard Keys
        case iGeKey::Apostrophe:
            return ImGuiKey_Apostrophe;
        case iGeKey::Comma:
            return ImGuiKey_Comma;
        case iGeKey::Minus:
            return ImGuiKey_Minus;
        case iGeKey::Period:
            return ImGuiKey_Period;
        case iGeKey::Slash:
            return ImGuiKey_Slash;
        case iGeKey::Semicolon:
            return ImGuiKey_Semicolon;
        case iGeKey::Equal:
            return ImGuiKey_Equal;
        case iGeKey::LeftBracket:
            return ImGuiKey_LeftBracket;
        case iGeKey::Backslash:
            return ImGuiKey_Backslash;
        case iGeKey::RightBracket:
            return ImGuiKey_RightBracket;
        case iGeKey::GraveAccent:
            return ImGuiKey_GraveAccent;

        default:
            IGE_CORE_WARN("iGeKey {} is not mapped in ImGuiKey!", keycode);
            return ImGuiKey_None; // Return None for any unrecognized keys
    }
}

/////////////////////////////////////////////////////////////////////////////
// ImGuiLayer ///////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
ImGuiLayer::ImGuiLayer() : Layer{"ImGuiLayer"} {}

ImGuiLayer::~ImGuiLayer() {}

void ImGuiLayer::OnAttach() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // when viewports are enabled we tweak WindowRounding/windowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    Application& app = Application::Get();
    GLFWwindow* window = static_cast<GLFWwindow*>(app.GetWindow().GetNativeWindow());

    // Setup Platform/Renderer bindings
    ImGui_ImplOpenGL3_Init("#version 410");
    ImGui_ImplGlfw_InitForOpenGL(window, true);
}

void ImGuiLayer::OnDetach() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

//void ImGuiLayer::OnEvent(Event& e) {}

void ImGuiLayer::Begin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::End() {
    ImGuiIO& io = ImGui::GetIO();
    Application& app = Application::Get();
    io.DisplaySize = ImVec2{(float) app.GetWindow().GetWidth(), (float) app.GetWindow().GetHeight()};

    // Rendering
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

} // namespace iGe