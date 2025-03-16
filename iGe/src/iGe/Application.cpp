#include "iGe/Application.h"

#include "iGe/Events/ApplicationEvent.h"
#include "iGe/Events/Event.h"
#include "iGe/Log.h"

namespace iGe
{

Application::Application() {}

Application::~Application() {}

void Application::Run() {
    WindowResizeEvent e(1280, 720);
    IGE_TRACE(e);

    while (true) {}
}

} // namespace iGe
