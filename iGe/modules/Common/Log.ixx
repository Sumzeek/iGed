module;
#include "iGeMacro.h"

export module iGe.Log;
import spdlog;
import iGe.Types;
import iGe.SmartPointer;

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

    static Ref<spdlog::logger>& GetCoreLogger() { return m_CoreLogger; }
    static Ref<spdlog::logger>& GetClientLogger() { return m_ClientLogger; }

private:
    Log() {}
    ~Log() {}

    static inline Ref<spdlog::logger> m_CoreLogger = nullptr;
    static inline Ref<spdlog::logger> m_ClientLogger = nullptr;
};

} // namespace iGe
