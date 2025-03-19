module;
#include "Common/Core.h"
#include "Common/iGepch.h"

export module iGe.Layer;

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
    virtual void OnEvent(Event& event);

    inline const std::string& GetName() const;

protected:
    std::string m_DebugName;
};

Layer::Layer(const std::string& name) : m_DebugName(name) {}
Layer::~Layer() {}

// ----------------- Layer::Implementation -----------------
void Layer::OnAttach() {}
void Layer::OnDetach() {}
void Layer::OnUpdate() {}
void Layer::OnEvent(Event& event) {}

const std::string& Layer::GetName() const { return m_DebugName; }

} // namespace iGe
