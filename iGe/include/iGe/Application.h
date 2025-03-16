#pragma once

#include "Core.h"

namespace iGe
{

class IGE_API Application {
public:
    Application();
    virtual ~Application();

    void Run();
};

// To be defined in CLIENT
Application* CreateApplication();

} // namespace iGe