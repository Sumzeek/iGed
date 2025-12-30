module;
#include "iGeMacro.h"

export module iGe.Core:Application;
import iGe.Common;
import iGe.Window;
import iGe.RHI;

int main(int argc, char** argv);

namespace iGe
{

export struct ApplicationCommandLineArgs {
    int32 Count = 0;
    char** Args = nullptr;

    const char* operator[](int32 index) const {
        Internal::Assert(index < Count);
        return Args[index];
    }
};

export struct ApplicationSpecification {
    string Name = "iGe Application";
    string WorkingDirectory;
    ApplicationCommandLineArgs CommandLineArgs;
};

class ImGuiLayer;
export class IGE_API Application {
public:
    Application(const ApplicationSpecification& specification);
    virtual ~Application();

    void Run();

    RHITexture* GetCurrentBackBufferTexture() const;
    RHITextureView* GetCurrentBackBufferView() const;

    void OnEvent(Event& e);

    void PushLayer(Ref<Layer> layer);
    void PushOverlay(Ref<Layer> layer);

    static Application& Get() { return *s_Instance; }
    Window& GetWindow() const { return *m_Window; }
    const ApplicationSpecification& GetSpecification() const { return m_Specification; }

private:
    bool OnWindowResizeEvent(WindowResizeEvent& event);
    bool OnWindowCloseEvent(WindowCloseEvent& event);

    void CreateSwapChain();
    void CreateCommandPool();
    void CreateInFlightResouce();

    static Application* s_Instance;

    Scope<RHISurface> m_Surface;
    Scope<RHISwapChain> m_SwapChain;

    // Per-frame resources
    static constexpr uint32 MAX_FRAMES_IN_FLIGHT = 2;
    uint32 m_CurrentFrame = 0;

    Scope<RHICommandPool> m_CommandPool;
    std::vector<Scope<RHICommandList>> m_CommandLists;
    std::vector<Scope<RHIFence>> m_InFlightFences;
    std::vector<Scope<RHISemaphore>> m_ImageAvailableSemaphores;
    std::vector<Scope<RHISemaphore>> m_RenderFinishedSemaphores;

    ApplicationSpecification m_Specification;
    Scope<Window> m_Window;
    bool m_Running = true;
    LayerStack m_LayerStack;
    float32 m_LastTime = 0.0f;
};

// ----------------- Application::Implementation -----------------
// To be defined in CLIENT
export IGE_API Application* CreateApplication(ApplicationCommandLineArgs args);

} // namespace iGe
