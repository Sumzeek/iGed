module;
#include "iGeMacro.h"

export module iGe.Core:ImGuiLayer;
import std;
import iGe.Layer;
import iGe.Event;
import iGe.Log;

namespace iGe
{

export class IGE_API ImGuiLayer : public Layer {
public:
    ImGuiLayer();
    ~ImGuiLayer();

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    //virtual void OnUpdate();
    virtual void OnImGuiRender() override;
    //virtual void OnEvent(Event& e);

    void Begin();
    void End();
};

} // namespace iGe
