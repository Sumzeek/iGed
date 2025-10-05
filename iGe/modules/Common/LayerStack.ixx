module;
#include "iGeMacro.h"

export module iGe.LayerStack;
import iGe.Types;
import iGe.Layer;
import iGe.SmartPointer;

namespace iGe
{
export class IGE_API LayerStack {
public:
    LayerStack() {}
    ~LayerStack() {}

    void PushLayer(Ref<Layer> layer) {
        m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex, layer);
        m_LayerInsertIndex++;
    }

    void PushOverlay(Ref<Layer> overlay) { m_Layers.emplace_back(overlay); }

    void PopLayer(Ref<Layer> layer) {
        auto it = std::find_if(m_Layers.begin(), m_Layers.end(),
                               [&](const Ref<Layer>& l) { return l.get() == layer.get(); });
        if (it != m_Layers.end()) {
            m_Layers.erase(it);
            m_LayerInsertIndex--;
        }
    }

    void PopOverlay(Ref<Layer> overlay) {
        auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
        if (it != m_Layers.end()) { m_Layers.erase(it); }
    }

    auto layers() noexcept { return std::views::all(m_Layers); }
    auto layers() const noexcept { return std::views::all(m_Layers); }

private:
    std::vector<Ref<Layer>> m_Layers;
    uint32 m_LayerInsertIndex = 0;
};
} // namespace iGe
