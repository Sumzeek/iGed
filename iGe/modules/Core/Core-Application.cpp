module;
#include "iGeMacro.h"

module iGe.Core;
import std;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// Application //////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

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
        for (Layer* layer: m_LayerStack) { layer->OnUpdate(); }

        m_ImGuiLayer->Begin();
        for (Layer* layer: m_LayerStack) { layer->OnImGuiRender(); }
        m_ImGuiLayer->End();

        m_Window->OnUpdate();
    }
}

void Application::OnEvent(Event& e) {
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

} // namespace iGe