module;
#include "iGeMacro.h"

export module iGe.Core:Application;
import std;
import iGe.Layer;
import iGe.LayerStack;
import iGe.Event;
import iGe.Window;
import iGe.SmartPointer;

namespace iGe
{

class ImGuiLayer;

export class IGE_API Application {
public:
    Application();
    virtual ~Application();

    void Run();

    void OnEvent(Event& e);
    bool OnWindowClose(Event& e);

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* layer);

    inline Window& GetWindow();
    static inline Application& Get();

private:
    static Application* s_Instance;

    std::unique_ptr<Window> m_Window;
    ImGuiLayer* m_ImGuiLayer;
    bool m_Running = true;
    LayerStack m_LayerStack;
    float m_LastTime = 0.0f;
};

// ----------------- Application::Implementation -----------------
// To be defined in CLIENT
export IGE_API Application* CreateApplication();

} // namespace iGe
