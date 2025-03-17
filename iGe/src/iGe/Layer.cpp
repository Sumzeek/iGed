#include "iGe/Layer.h"
#include "iGepch.h"

namespace iGe
{

Layer::Layer(const std::string& name) : m_DebugName(name) {}

Layer::~Layer() {}

const std::string& Layer::GetName() const { return m_DebugName; }

} // namespace iGe
