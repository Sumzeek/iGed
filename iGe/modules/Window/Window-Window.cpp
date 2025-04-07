module;
#include "iGeMacro.h"

module iGe.Window;
import :Window;
import :WindowsWindow;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// Window ///////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Window::~Window() {}

Scope<Window> Window::Create(const WindowProps& props) {
#ifdef IGE_PLATFORM_WINDOWS
    return CreateScope<WindowsWindow>(props);
#else
    IGE_CORE_ASSERT(false, "Unknown platform!");
    return nullptr;
#endif
}

} // namespace iGe