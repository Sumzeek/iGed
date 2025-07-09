module;
#include "iGeMacro.h"

export module iGe.Layer;

import std;
import iGe.Event;
import iGe.Timestep;

namespace iGe
{

export class IGE_API Layer {
public:
    Layer(const std::string& name = "Layer");
    virtual ~Layer();

    virtual void OnAttach();
    virtual void OnDetach();
    virtual void OnUpdate(Timestep ts);
    virtual void OnImGuiRender();
    virtual void OnEvent(Event& event);

    inline const std::string& GetName() const;

protected:
    std::string m_DebugName;
};

} // namespace iGe
