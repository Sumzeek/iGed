module;
#include "iGeMacro.h"

module iGe.Layer;

namespace iGe
{

// ---------------------------------- Layer::Implementation ----------------------------------
/////////////////////////////////////////////////////////////////////////////
// Layer ////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Layer::Layer(const std::string& name) : m_DebugName(name) {}

Layer::~Layer() {}

void Layer::OnAttach() {}

void Layer::OnDetach() {}

void Layer::OnUpdate(Timestep ts) {}

void Layer::OnImGuiRender() {}

void Layer::OnEvent(Event& event) {}

const std::string& Layer::GetName() const { return m_DebugName; }

} // namespace iGe
