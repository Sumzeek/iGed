module;
#include "iGeMacro.h"

export module iGe.Core:Input;
import std;

namespace iGe
{

export class IGE_API Input {
public:
    static bool IsKeyPressed(int keycode) { return s_Instance->IsKeyPressedImpl(keycode); }
    static bool IsMouseButtonPressed(int button) { return s_Instance->IsMouseButtonPressedImpl(button); }
    static std::pair<float, float> GetMousePosition() { return s_Instance->GetMousePositionImpl(); }
    static float GetMouseX() { return s_Instance->GetMouseXImpl(); }
    static float GetMouseY() { return s_Instance->GetMouseYImpl(); }

protected:
    virtual bool IsKeyPressedImpl(int keycode) = 0;
    virtual bool IsMouseButtonPressedImpl(int button) = 0;
    virtual std::pair<float, float> GetMousePositionImpl() = 0;
    virtual float GetMouseXImpl() = 0;
    virtual float GetMouseYImpl() = 0;

    static Input* s_Instance;
};

} // namespace iGe
