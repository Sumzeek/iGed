module;
#include "iGeMacro.h"

export module iGe.Core:Application;
import std;
import iGe.Layer;
import iGe.Log;
import iGe.LayerStack;
import iGe.Event;
import iGe.Window;
import iGe.SmartPointer;

int main(int argc, char** argv);

namespace iGe
{

export struct ApplicationCommandLineArgs {
    int Count = 0;
    char** Args = nullptr;

    const char* operator[](int index) const {
        IGE_CORE_ASSERT(index < Count);
        return Args[index];
    }
};

export struct ApplicationSpecification {
    std::string Name = "Hazel Application";
    std::string WorkingDirectory;
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

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* layer);

    inline Window& GetWindow() { return *m_Window; }
    static inline Application& Get(){ return *s_Instance; }
    const ApplicationSpecification& GetSpecification() const;

private:
    static Application* s_Instance;

    ApplicationSpecification m_Specification;
    std::unique_ptr<Window> m_Window;
    ImGuiLayer* m_ImGuiLayer;
    bool m_Running = true;
    LayerStack m_LayerStack;
    float m_LastTime = 0.0f;
};

// ----------------- Application::Implementation -----------------
// To be defined in CLIENT
export IGE_API Application* CreateApplication(ApplicationCommandLineArgs args);

} // namespace iGe
