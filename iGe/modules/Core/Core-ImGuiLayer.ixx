module;
#include "iGeMacro.h"

export module iGe.Core:ImGuiLayer;
import iGe.Common;

namespace iGe
{
export class IGE_API ImGuiLayer : public Layer {
public:
    ImGuiLayer() : Layer{"ImGuiLayer"} {}
    ~ImGuiLayer() {}

    static void Begin();
    static void End();

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnEvent(Event& e) override {}
};
} // namespace iGe
