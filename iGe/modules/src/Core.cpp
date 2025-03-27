module;
#include "Macro.h"

// TEMPORARY
#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

module iGe.Core;

namespace iGe
{
// ---------------------------------- Application::Implementation ----------------------------------
Application* Application::s_Instance = nullptr;

Application::Application() {
    IGE_CORE_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;

    m_Window = std::unique_ptr<Window>(Window::Create());
    m_Window->SetEventCallback(IGE_BIND_EVENT_FN(Application::OnEvent));
    m_Running = true;

    m_ImGuiLayer = new ImGuiLayer{};
    PushOverlay(m_ImGuiLayer);
}

Application::~Application() {}

void Application::Run() {
    while (m_Running) {
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        for (Layer* layer: m_LayerStack) { layer->OnUpdate(); }

        m_ImGuiLayer->Begin();
        for (Layer* layer: m_LayerStack) { layer->OnImGuiRender(); }
        m_ImGuiLayer->End();

        m_Window->OnUpdate();
    }
}

void Application::OnEvent(Event& e) {
    //IGE_CORE_TRACE(e);
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(IGE_BIND_EVENT_FN(Application::OnWindowClose));

    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it) {
        if (e.m_Handled) { break; }
        (*it)->OnEvent(e);
    }
}

bool Application::OnWindowClose(Event& e) {
    m_Running = false;
    return true;
}

void Application::PushLayer(Layer* layer) {
    m_LayerStack.PushLayer(layer);
    layer->OnAttach();
}

void Application::PushOverlay(Layer* layer) {
    m_LayerStack.PushOverlay(layer);
    layer->OnAttach();
}

Window& Application::GetWindow() { return *m_Window; }

Application& Application::Get() { return *s_Instance; }

// ---------------------------------- ImGuiLayer::Implementation ----------------------------------
ImGuiKey iGeKeyToImGuiKey(iGeKey keycode);

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

//void ImGuiLayer::OnUpdate() {
//    ImGuiIO& io = ImGui::GetIO();
//    Application& app = Application::Get();
//    io.DisplaySize = ImVec2{(float) app.GetWindow().GetWidth(), (float) app.GetWindow().GetHeight()};
//
//    float time = (float) glfwGetTime();
//    io.DeltaTime = m_Time > 0.0f ? (time - m_Time) : (1.0f / 60.0f);
//    m_Time = time;
//
//    ImGui_ImplOpenGL3_NewFrame();
//    ImGui::NewFrame();
//
//    static bool show = true;
//    ImGui::ShowDemoWindow(&show);
//
//    ImGui::Render();
//    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
//}

void ImGuiLayer::OnImGuiRender() {
    static bool show = true;
    ImGui::ShowDemoWindow(&show);
}

//void ImGuiLayer::OnEvent(Event& e) {
//    EventDispatcher dispatcher(e);
//    dispatcher.Dispatch<MouseMoveEvent>(IGE_BIND_EVENT_FN(ImGuiLayer::OnMouseMovedEvent));
//    dispatcher.Dispatch<MouseScrolledEvent>(IGE_BIND_EVENT_FN(ImGuiLayer::OnMouseScrolledEvent));
//    dispatcher.Dispatch<MouseButtonPressedEvent>(IGE_BIND_EVENT_FN(ImGuiLayer::OnMouseButtonPressedEvent));
//    dispatcher.Dispatch<MouseButtonReleasedEvent>(IGE_BIND_EVENT_FN(ImGuiLayer::OnMouseButtonReleasedEvent));
//    dispatcher.Dispatch<KeyPressedEvent>(IGE_BIND_EVENT_FN(ImGuiLayer::OnKeyPressedEvent));
//    dispatcher.Dispatch<KeyReleasedEvent>(IGE_BIND_EVENT_FN(ImGuiLayer::OnKeyReleasedEvent));
//    dispatcher.Dispatch<KeyTypedEvent>(IGE_BIND_EVENT_FN(ImGuiLayer::OnKeyTypedEvent));
//    dispatcher.Dispatch<WindowResizeEvent>(IGE_BIND_EVENT_FN(ImGuiLayer::OnWindowResizedEvent));
//}

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

//bool ImGuiLayer::OnMouseMovedEvent(MouseMoveEvent& e) {
//    ImGuiIO& io = ImGui::GetIO();
//    io.AddMousePosEvent(e.GetX(), e.GetY());
//
//    return false;
//}
//
//bool ImGuiLayer::OnMouseScrolledEvent(MouseScrolledEvent& e) {
//    ImGuiIO& io = ImGui::GetIO();
//    io.AddMouseWheelEvent(e.GetXOffset(), e.GetYOffset());
//
//    return false;
//}
//
//bool ImGuiLayer::OnMouseButtonPressedEvent(MouseButtonPressedEvent& e) {
//    ImGuiIO& io = ImGui::GetIO();
//
//    auto button = static_cast<int>(e.GetMouseButton());
//    io.AddMouseButtonEvent(button, true);
//
//    return false;
//}
//
//bool ImGuiLayer::OnMouseButtonReleasedEvent(MouseButtonReleasedEvent& e) {
//    ImGuiIO& io = ImGui::GetIO();
//
//    auto button = static_cast<int>(e.GetMouseButton());
//    io.AddMouseButtonEvent(button, false);
//
//    return false;
//}
//
//bool ImGuiLayer::OnKeyPressedEvent(KeyPressedEvent& e) {
//    ImGuiIO& io = ImGui::GetIO();
//
//    auto iGeKeycode = e.GetKeyCode();
//    auto imgui_key = iGeKeyToImGuiKey(e.GetKeyCode());
//    //int scancode = -1;
//
//    if (imgui_key != ImGuiKey_None) {
//        io.AddKeyEvent(imgui_key, true);
//        //io.SetKeyEventNativeData(imgui_key, keycode, scancode);
//    } else {
//        IGE_CORE_WARN("Unsupported key pressed: {}", iGeKeycode);
//    }
//    return false;
//}
//
//bool ImGuiLayer::OnKeyReleasedEvent(KeyReleasedEvent& e) {
//    ImGuiIO& io = ImGui::GetIO();
//
//    auto iGeKeycode = e.GetKeyCode();
//    auto imgui_key = iGeKeyToImGuiKey(iGeKeycode);
//    //int scancode = -1;
//
//    if (imgui_key != ImGuiKey_None) {
//        io.AddKeyEvent(imgui_key, false);
//        //io.SetKeyEventNativeData(imgui_key, keycode, scancode);
//    } else {
//        IGE_CORE_WARN("Unsupported key released: {}", iGeKeycode);
//    }
//    return false;
//
//    return false;
//}
//
//bool ImGuiLayer::OnKeyTypedEvent(KeyTypedEvent& e) {
//    ImGuiIO& io = ImGui::GetIO();
//
//    int codepoint = e.GetCodePoint();
//    if (codepoint > 0 && codepoint < 0x10000) { io.AddInputCharacter(codepoint); }
//
//    return false;
//}
//
//bool ImGuiLayer::OnWindowResizedEvent(WindowResizeEvent& e) {
//    ImGuiIO& io = ImGui::GetIO();
//    io.DisplaySize = ImVec2{(float) e.GetWidth(), (float) e.GetHeight()};
//    io.DisplayFramebufferScale = ImVec2{1.0f, 1.0f};
//    //glViewport(0, 0, e.GetWidth(), e.GetHeight());
//
//    return false;
//}

ImGuiKey iGeKeyToImGuiKey(iGeKey keycode) {
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

// ---------------------------------- WindowsInput::Implementation ----------------------------------
Input* Input::s_Instance = new WindowsInput{};

bool WindowsInput::IsKeyPressedImpl(int keycode) {
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto state = glfwGetKey(window, keycode);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool WindowsInput::IsMouseButtonPressedImpl(int button) {
    auto window = static_cast<GLFWwindow*>(Application::Get().GetWindow().GetNativeWindow());
    auto state = glfwGetMouseButton(window, button);
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
