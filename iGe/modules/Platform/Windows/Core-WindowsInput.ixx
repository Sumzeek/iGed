module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "iGeMacro.h"

export module iGe.Core:WindowsInput;
import :Input;
import iGe.Common;

namespace iGe
{

export class IGE_API WindowsInput : public Input {
protected:
    virtual bool IsKeyPressedImpl(iGeKey keycode) override;
    virtual bool IsMouseButtonPressedImpl(iGeKey button) override;
    virtual std::pair<float, float> GetMousePositionImpl() override;
    virtual float32 GetMouseXImpl() override;
    virtual float32 GetMouseYImpl() override;
};

} // namespace iGe
#endif
