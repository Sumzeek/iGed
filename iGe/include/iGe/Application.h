#pragma once

#include "iGe/Core.h"
#include "iGe/Window.h"

namespace iGe
{

class IGE_API Application {
public:
    Application();
    virtual ~Application();

    void Run();

private:
    std::unique_ptr<Window> m_Window;
    bool m_Running;
};

// To be defined in CLIENT
Application* CreateApplication();

} // namespace iGe
