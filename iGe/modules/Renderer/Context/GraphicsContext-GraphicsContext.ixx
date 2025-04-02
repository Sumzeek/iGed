module;
#include "iGeMacro.h"

export module iGe.GraphicsContext:GraphicsContext;

namespace iGe
{

export class IGE_API GraphicsContext {
public:
    virtual void Init() = 0;
    virtual void SwapBuffers() = 0;
};

} // namespace iGe