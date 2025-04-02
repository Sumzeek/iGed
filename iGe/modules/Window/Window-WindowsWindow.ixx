module;
#include "iGeMacro.h"

#include <glad/gl.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

export module iGe.Window:Windows;
import std;
import :Window;
import iGe.Event;
import iGe.Log;
import iGe.GraphicsContext;

namespace iGe
{

export class IGE_API WindowsWindow : public Window {
public:
    WindowsWindow(const WindowProps& props);
    virtual ~WindowsWindow();

    void OnUpdate() override;

    virtual unsigned int GetWidth() const override;
    virtual unsigned int GetHeight() const override;

    // Window attributes
    virtual void SetEventCallback(const EventCallbackFn& callback) override;
    virtual void SetVSync(bool enable) override;
    virtual bool IsVSync() const override;

    virtual void* GetNativeWindow() const override;

private:
    virtual void Init(const WindowProps& props);
    virtual void ShutDown();

    struct WindowData {
        std::string Title;
        unsigned int Width;
        unsigned int Height;
        bool VSync;

        EventCallbackFn EventCallback;
    };

    GLFWwindow* m_Window;
    GraphicsContext* m_Context;
    WindowData m_Data;
};

} // namespace iGe
