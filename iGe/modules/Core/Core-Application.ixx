module;
#include "iGeMacro.h"

export module iGe.Core:Application;
import iGe.Common;
import iGe.Window;

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

    void OnEvent(Event& e);
    bool OnWindowClose(Event& e);

    void PushLayer(Ref<Layer> layer);
    void PushOverlay(Ref<Layer> layer);

    static Application& Get() { return *s_Instance; }
    Window& GetWindow() const { return *m_Window; }
    const ApplicationSpecification& GetSpecification() const { return m_Specification; }

private:
    static Application* s_Instance;

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
