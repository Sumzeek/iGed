#pragma once

#include "iGe/Core.h"
#include "iGe/Events/Event.h"

namespace iGe
{

class IGE_API Layer {
public:
    Layer(const std::string& name = "Layer");
    virtual ~Layer();

    virtual void OnAttach() {}
    virtual void OnDetach() {}
    virtual void OnUpdate() {}
    virtual void OnEvent(Event& event) {}

    inline const std::string& GetName() const;

protected:
    std::string m_DebugName;
};

} // namespace iGe
