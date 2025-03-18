#include "Common/Core.h"
#include "Common/iGepch.h"

export module iGe.LayerStack;

import iGe.Layer;

namespace iGe
{

export class IGE_API LayerStack {
public:
    LayerStack();
    ~LayerStack();

    void PushLayer(Layer* layer);
    void PushOverlay(Layer* overlay);
    void PopLayer(Layer* layer);
    void PopOverlay(Layer* overlay);

    std::vector<Layer*>::iterator begin();
    std::vector<Layer*>::iterator end();
    std::vector<Layer*>::reverse_iterator rbegin();
    std::vector<Layer*>::reverse_iterator rend();

    std::vector<Layer*>::const_iterator begin() const;
    std::vector<Layer*>::const_iterator end() const;
    std::vector<Layer*>::const_reverse_iterator rbegin() const;
    std::vector<Layer*>::const_reverse_iterator rend() const;

private:
    std::vector<Layer*> m_Layers;
    std::vector<Layer*>::iterator m_LayerInsert;
};

LayerStack::LayerStack() { m_LayerInsert = m_Layers.begin(); }

LayerStack::~LayerStack() {
    for (Layer* layer: m_Layers) { delete layer; }
}

void LayerStack::PushLayer(Layer* layer) { m_LayerInsert = m_Layers.emplace(m_LayerInsert, layer); }

void LayerStack::PushOverlay(Layer* overlay) { m_Layers.emplace_back(overlay); }

void LayerStack::PopLayer(Layer* layer) {
    auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
    if (it != m_Layers.end()) {
        m_Layers.erase(it);
        m_LayerInsert--;
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
