module iGe.LayerStack;

namespace iGe
{
////////////////////////////////////////////////////////////////////
//////////////////// LayerStack::Implementation ////////////////////
////////////////////////////////////////////////////////////////////
LayerStack::LayerStack() {}

LayerStack::~LayerStack() {
    for (Layer* layer: m_Layers) { delete layer; }
}

void LayerStack::PushLayer(Layer* layer) {
    m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
    m_LayerInsertIndex++;
}

void LayerStack::PushOverlay(Layer* overlay) { m_Layers.emplace_back(overlay); }

void LayerStack::PopLayer(Layer* layer) {
    auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
    if (it != m_Layers.end()) {
        m_Layers.erase(it);
        m_LayerInsertIndex--;
    }
}

void LayerStack::PopOverlay(Layer* overlay) {
    auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
    if (it != m_Layers.end()) { m_Layers.erase(it); }
}

std::vector<Layer*>::iterator LayerStack::begin() { return m_Layers.begin(); }
std::vector<Layer*>::iterator LayerStack::end() { return m_Layers.end(); }
std::vector<Layer*>::reverse_iterator LayerStack::rbegin() { return m_Layers.rbegin(); }
std::vector<Layer*>::reverse_iterator LayerStack::rend() { return m_Layers.rend(); }

std::vector<Layer*>::const_iterator LayerStack::begin() const { return m_Layers.begin(); }
std::vector<Layer*>::const_iterator LayerStack::end() const { return m_Layers.end(); }
std::vector<Layer*>::const_reverse_iterator LayerStack::rbegin() const { return m_Layers.rbegin(); }
std::vector<Layer*>::const_reverse_iterator LayerStack::rend() const { return m_Layers.rend(); }

} // namespace iGe