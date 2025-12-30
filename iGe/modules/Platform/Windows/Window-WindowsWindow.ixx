module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"
    #define GLFW_INCLUDE_NONE
    #include <GLFW/glfw3.h>
    #define GLFW_EXPOSE_NATIVE_WIN32
    #include <GLFW/glfw3native.h>

export module iGe.Window:WindowsWindow;
import :Window;
import iGe.Common;
import iGe.Renderer;

namespace iGe
{

export class IGE_API WindowsWindow : public Window {
public:
    WindowsWindow(const WindowProps& props);
    virtual ~WindowsWindow();

    void OnUpdate() override;

    virtual uint32 GetWidth() const override { return m_Data.Width; }
    virtual uint32 GetHeight() const override { return m_Data.Height; }

    // Window attributes
    virtual void SetEventCallback(const EventCallbackFn& callback) override { m_Data.EventCallback = callback; }
    virtual void SetVSync(bool enable) override;
    virtual bool IsVSync() const override { return m_Data.VSync; }

    virtual void* GetNativeWindow() const override { return m_Window; }
    virtual void* GetNativeWindowHandle() const override { return glfwGetWin32Window(m_Window); };

private:
    virtual void Init(const WindowProps& props);
    virtual void ShutDown();

    struct WindowData {
        string Title;
        uint32 Width;
        uint32 Height;
        bool VSync;

        EventCallbackFn EventCallback;
    };

    GLFWwindow* m_Window;
    WindowData m_Data;
};

} // namespace iGe
#endif
