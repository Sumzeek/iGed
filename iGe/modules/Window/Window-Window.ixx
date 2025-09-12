module;
#include "iGeMacro.h"

export module iGe.Window:Window;
import iGe.Common;

namespace iGe
{
export struct WindowProps {
    std::string Title;
    uint32 Width;
    uint32 Height;

    WindowProps(const std::string& title = "iGame Game Engine", uint32 width = 1280, uint32 height = 720)
        : Title(title), Width(width), Height(height) {}
};

using EventCallbackFn = std::function<void(Event&)>;

// Interface representing a desktop system based Window
export class IGE_API Window {
public:
    virtual ~Window();

    virtual void OnUpdate() = 0;

    virtual uint32 GetWidth() const = 0;
    virtual uint32 GetHeight() const = 0;

    // Window attributes
    virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
    virtual void SetVSync(bool enable) = 0;
    virtual bool IsVSync() const = 0;

    virtual void* GetNativeWindow() const = 0;

    static Scope<Window> Create(const WindowProps& props = WindowProps());
};
} // namespace iGe
