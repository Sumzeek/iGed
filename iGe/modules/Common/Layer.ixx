module;
#include "iGeMacro.h"

export module iGe.Layer;
import iGe.Types;
import iGe.Event;
import iGe.Timestep;

namespace iGe
{
export class IGE_API Layer {
public:
    Layer(const std::string& name = "Layer") : m_DebugName(name) {}
    virtual ~Layer() {}

    virtual void OnAttach() {}
    virtual void OnDetach() {}
    virtual void OnUpdate(Timestep ts) {}
    virtual void OnImGuiRender() {}
    virtual void OnEvent(Event& event) {}

    inline const std::string& GetName() const { return m_DebugName; }

protected:
    std::string m_DebugName;
};
} // namespace iGe
