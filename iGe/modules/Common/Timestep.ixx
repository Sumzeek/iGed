module;
#include "iGeMacro.h"

export module iGe.Timestep;

namespace iGe
{

export class IGE_API Timestep {
public:
    Timestep(float time = 0.0f);

    operator float() const;

    float GetSeconds() const;
    float GetMilliseconds() const;

private:
    float m_Time;
};

} // namespace iGe
