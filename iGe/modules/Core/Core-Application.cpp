module;
#include "iGeMacro.h"

module iGe.Core;
import :Application;

import std;
import iGe.Layer;
import iGe.Log;
import iGe.Event;
import iGe.Timestep;
import iGe.Renderer;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// Application //////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& specification) : m_Specification{specification} {
    IGE_CORE_ASSERT(!s_Instance, "Application already exists!");
    s_Instance = this;

    m_Window = std::unique_ptr<Window>(Window::Create());
    m_Window->SetEventCallback(IGE_BIND_EVENT_FN(Application::OnEvent));

    Renderer::Init();

    m_ImGuiLayer = new ImGuiLayer{};
    PushOverlay(m_ImGuiLayer);
}

Application::~Application() {}

void Application::Run() {
    while (m_Running) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        Timestep timestep{time - m_LastTime};
        m_LastTime = time;

        for (Layer* layer: m_LayerStack) { layer->OnUpdate(timestep); }

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

const ApplicationSpecification& Application::GetSpecification() const { return m_Specification; }

} // namespace iGe