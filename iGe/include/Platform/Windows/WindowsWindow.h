#pragma once

#include "iGe/Window.h"

#include <GLFW/glfw3.h>

namespace iGe
{

class WindowsWindow : public Window {
public:
    WindowsWindow(const WindowProps& props);
    virtual ~WindowsWindow();

    void OnUpdate() override;

    virtual unsigned int GetWidth() const override;
    virtual unsigned int GetHeight() const override;

    // Window attributes
    void SetEventCallback(const EventCallbackFn& callback) override;
    void SetVSync(bool enable) override;
    bool IsVSync() const override;

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
    WindowData m_Data;
};

} // namespace iGe
