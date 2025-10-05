module;
#include "iGeMacro.h"

export module iGe.Core:Input;
import iGe.Common;

namespace iGe
{
export class IGE_API Input {
public:
    static bool IsKeyPressed(iGeKey keycode) { return s_Instance->IsKeyPressedImpl(keycode); }
    static bool IsMouseButtonPressed(iGeKey button) { return s_Instance->IsMouseButtonPressedImpl(button); }
    static std::pair<float32, float32> GetMousePosition() { return s_Instance->GetMousePositionImpl(); }
    static float32 GetMouseX() { return s_Instance->GetMouseXImpl(); }
    static float32 GetMouseY() { return s_Instance->GetMouseYImpl(); }

protected:
    virtual bool IsKeyPressedImpl(iGeKey keycode) = 0;
    virtual bool IsMouseButtonPressedImpl(iGeKey button) = 0;
    virtual std::pair<float, float> GetMousePositionImpl() = 0;
    virtual float32 GetMouseXImpl() = 0;
    virtual float32 GetMouseYImpl() = 0;

    static Input* s_Instance;
};
} // namespace iGe
