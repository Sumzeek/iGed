#pragma once

#include "Core.h"

namespace iGed
{

class IGED_API Application {
public:
    Application();
    virtual ~Application();

    void Run();
};

// To be defined in CLIENT
Application* CreateApplication();

} // namespace iGed