#include "iGe/Log.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace iGe
{

std::shared_ptr<spdlog::logger> Log::m_CoreLogger;
std::shared_ptr<spdlog::logger> Log::m_ClientLogger;

Log::Log() {}

Log::~Log() {}

void Log::Init() {
    spdlog::set_pattern("%^[%T] %n: %v%$");

    m_CoreLogger = spdlog::stdout_color_mt("iGe");
    m_CoreLogger->set_level(spdlog::level::trace);
    m_ClientLogger = spdlog::stdout_color_mt("APP");
    m_ClientLogger->set_level(spdlog::level::trace);
}

std::shared_ptr<spdlog::logger>& Log::GetCoreLogger() { return m_CoreLogger; }

std::shared_ptr<spdlog::logger>& Log::GetClientLogger() { return m_ClientLogger; }

} // namespace iGe