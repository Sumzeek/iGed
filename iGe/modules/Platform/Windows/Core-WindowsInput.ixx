module;
#include "iGeMacro.h"

export module iGe.Core:WindowsInput;
import std;
import :Input;

namespace iGe
{

export class IGE_API WindowsInput : public Input {
protected:
    virtual bool IsKeyPressedImpl(iGeKey keycode) override;
    virtual bool IsMouseButtonPressedImpl(iGeKey button) override;
    virtual std::pair<float, float> GetMousePositionImpl() override;
    virtual float GetMouseXImpl() override;
    virtual float GetMouseYImpl() override;
};

} // namespace iGe
