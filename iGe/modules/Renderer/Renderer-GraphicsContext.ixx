module;
#include "iGeMacro.h"

export module iGe.Renderer:GraphicsContext;
import iGe.Common;

namespace iGe
{
export class IGE_API GraphicsContext {
public:
    virtual ~GraphicsContext() = default;

    virtual void Init() = 0;
    virtual void SwapBuffers() = 0;

    static Scope<GraphicsContext> Create(void* window);
};
} // namespace iGe
