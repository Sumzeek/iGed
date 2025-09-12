module;
#include "iGeMacro.h"

export module iGe.Log;
import iGe.Types;
import iGe.SmartPointer;
import spdlog;

namespace iGe
{
export class IGE_API Log {
public:
    static void Init() {
        spdlog::set_pattern("%^[%T] %n: %v%$");

        m_CoreLogger = spdlog::stdout_color_mt("iGe");
        m_CoreLogger->set_level(spdlog::level::trace);
        m_ClientLogger = spdlog::stdout_color_mt("APP");
        m_ClientLogger->set_level(spdlog::level::trace);
    }

    inline static Ref<spdlog::logger>& GetCoreLogger() { return m_CoreLogger; }
    inline static Ref<spdlog::logger>& GetClientLogger() { return m_ClientLogger; }

private:
    Log() {}
    ~Log() {}

    static Ref<spdlog::logger> m_CoreLogger;
    static Ref<spdlog::logger> m_ClientLogger;
};
} // namespace iGe
