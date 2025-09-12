module;
#include "iGeMacro.h"

export module iGe.Timestep;
import iGe.Types;

namespace iGe
{
export class IGE_API Timestep {
public:
    Timestep(float32 time = 0.0f) : m_Time(time) {}

    operator float() const { return m_Time; }

    inline float32 GetSeconds() const { return m_Time; }
    inline float32 GetMilliseconds() const { return m_Time * 1000.0f; }

private:
    float32 m_Time;
};
} // namespace iGe
