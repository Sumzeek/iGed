module iGe.Core;
import :Application;
import iGe.Renderer;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// Application //////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& specification) : m_Specification{specification} {
    Internal::Assert(!s_Instance, "Application already exists!");
    s_Instance = this;

    m_Window = std::unique_ptr<Window>(Window::Create());
    m_Window->SetEventCallback(std::bind(&Application::OnEvent, this, std::placeholders::_1));

    Renderer::Init();

    m_ImGuiLayer = new ImGuiLayer{};
    PushOverlay(m_ImGuiLayer);
}

Application::~Application() {}

void Application::Run() {
    while (m_Running) {
        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float32 time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        Timestep timestep{time - m_LastTime};
        m_LastTime = time;

        for (Layer* layer: m_LayerStack.layers()) { layer->OnUpdate(timestep); }

        m_ImGuiLayer->Begin();
        for (Layer* layer: m_LayerStack.layers()) { layer->OnImGuiRender(); }
        m_ImGuiLayer->End();

        m_Window->OnUpdate();
    }
}

void Application::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowCloseEvent>(std::bind(&Application::OnWindowClose, this, std::placeholders::_1));

    for (Layer* layer: m_LayerStack.layers() | std::views::reverse) {
        if (e.m_Handled) break;
        layer->OnEvent(e);
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
} // namespace iGe
