module;
#include "Macro.h"

export module iGe.Layer;
import std;
import iGe.Event;

namespace iGe
{

export class IGE_API Layer {
public:
    Layer(const std::string& name = "Layer");
    virtual ~Layer();

    virtual void OnAttach();
    virtual void OnDetach();
    virtual void OnUpdate();
    virtual void OnImGuiRender();
    virtual void OnEvent(Event& event);

    inline const std::string& GetName() const;

protected:
    std::string m_DebugName;
};

} // namespace iGe
