module;
#include "iGeMacro.h"

export module iGe.Log;
import std;
import spdlog;
import iGe.SmartPointer;

namespace iGe
{

export class IGE_API Log {
public:
    static void Init();

    static Ref<spdlog::logger>& GetCoreLogger();
    static Ref<spdlog::logger>& GetClientLogger();

private:
    Log();
    ~Log();

    static Ref<spdlog::logger> m_CoreLogger;
    static Ref<spdlog::logger> m_ClientLogger;
};

} // namespace iGe
