#include "iGe/Application.h"
#include "iGepch.h"

#include "iGe/Events/ApplicationEvent.h"

#include <GLFW/glfw3.h>

namespace iGe
{

#define BIND_EVENT_FN(x) std::bind(&x, this, std::placeholders::_1)

Application::Application() {
    m_Window = std::unique_ptr<Window>(Window::Create());
    m_Window->SetEventCallback(BIND_EVENT_FN(Application::OnEvent));
    m_Running = true;
}

Application::~Application() {}

void Application::Run() {
    while (m_Running) {
        glClearColor(1.0f, 0.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        for (Layer* layer: m_LayerStack) { layer->OnUpdate(); }

        m_Window->OnUpdate();
    }
}

void Application::OnEvent(Event& e) {
    //IGE_CORE_TRACE(e);
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT_FN(Application::OnWindowClose));

    for (auto it = m_LayerStack.rbegin(); it != m_LayerStack.rend(); ++it) {
        if (e.m_Handled) { break; }
        (*it)->OnEvent(e);
    }
}

bool Application::OnWindowClose(Event& e) {
    m_Running = false;
    return true;
}

void Application::PushLayer(Layer* layer) { m_LayerStack.PushLayer(layer); }

void Application::PushOverlay(Layer* overlay) { m_LayerStack.PushOverlay(overlay); }

} // namespace iGe
