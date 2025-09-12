module;
#include "iGeMacro.h"

export module iGe.LayerStack;
import iGe.Types;
import iGe.Layer;

namespace iGe
{
export class IGE_API LayerStack {
public:
    LayerStack() {}
    ~LayerStack() {
        for (Layer* layer: m_Layers) { delete layer; }
    }

    void PushLayer(Layer* layer) {
        m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
        m_LayerInsertIndex++;
    }

    void PushOverlay(Layer* overlay) { m_Layers.emplace_back(overlay); }

    void PopLayer(Layer* layer) {
        auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
        if (it != m_Layers.end()) {
            m_Layers.erase(it);
            m_LayerInsertIndex--;
        }
    }

    void PopOverlay(Layer* overlay) {
        auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
        if (it != m_Layers.end()) { m_Layers.erase(it); }
    }

    auto layers() noexcept { return std::views::all(m_Layers); }
    auto layers() const noexcept { return std::views::all(m_Layers); }

private:
    std::vector<Layer*> m_Layers;
    uint32 m_LayerInsertIndex = 0;
};
} // namespace iGe
