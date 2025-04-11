module iGe.Log;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// Log //////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Ref<spdlog::logger> Log::m_CoreLogger = nullptr;
Ref<spdlog::logger> Log::m_ClientLogger = nullptr;

Log::Log() {}

Log::~Log() {}

void Log::Init() {
    spdlog::set_pattern("%^[%T] %n: %v%$");

    m_CoreLogger = spdlog::stdout_color_mt("iGe");
    m_CoreLogger->set_level(spdlog::level::trace);
    m_ClientLogger = spdlog::stdout_color_mt("APP");
    m_ClientLogger->set_level(spdlog::level::trace);
}

Ref<spdlog::logger>& Log::GetCoreLogger() { return m_CoreLogger; }

Ref<spdlog::logger>& Log::GetClientLogger() { return m_ClientLogger; }

} // namespace iGe
