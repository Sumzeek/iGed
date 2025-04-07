module;
#include "iGeMacro.h"

export module iGe.Window:Window;
import std;
import iGe.Event;
import iGe.SmartPointer;

namespace iGe
{

export struct WindowProps {
    std::string Title;
    unsigned int Width;
    unsigned int Height;

    WindowProps(const std::string& title = "iGame Game Engine", unsigned int width = 1280, unsigned int height = 720)
        : Title(title), Width(width), Height(height) {}
};

using EventCallbackFn = std::function<void(Event&)>;

// Interface representing a desktop system based Window
export class IGE_API Window {
public:
    virtual ~Window();

    virtual void OnUpdate() = 0;

    virtual unsigned int GetWidth() const = 0;
    virtual unsigned int GetHeight() const = 0;

    // Window attributes
    virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
    virtual void SetVSync(bool enable) = 0;
    virtual bool IsVSync() const = 0;

    virtual void* GetNativeWindow() const = 0;

    static Scope<Window> Create(const WindowProps& props = WindowProps());
};

} // namespace iGe
