#pragma once

#include "iGe/Core.h"
#include "iGe/LayerStack.h"
#include "iGe/Window.h"

namespace iGe
{

class IGE_API Application {
public:
    Application();
    virtual ~Application();

    void Run();

    void OnEvent(Event& e);
    bool OnWindowClose(Event& e);

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);

private:
    std::unique_ptr<Window> m_Window;
    bool m_Running;
    LayerStack m_LayerStack;
};

// To be defined in CLIENT
Application* CreateApplication();

} // namespace iGe
