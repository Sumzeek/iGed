#pragma once

#include "iGe/Core.h"
#include "spdlog/fmt/ostr.h" // enable << support
#include "spdlog/spdlog.h"

namespace iGe
{

class IGE_API Log {
public:
    static void Init();

    static std::shared_ptr<spdlog::logger>& GetCoreLogger();
    static std::shared_ptr<spdlog::logger>& GetClientLogger();

private:
    Log();
    ~Log();

    static std::shared_ptr<spdlog::logger> m_CoreLogger;
    static std::shared_ptr<spdlog::logger> m_ClientLogger;
};

} // namespace iGe

#define IGE_CORE_TRACE(...) ::iGe::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define IGE_CORE_DEBUG(...) ::iGe::Log::GetCoreLogger()->debug(__VA_ARGS__)
#define IGE_CORE_INFO(...) ::iGe::Log::GetCoreLogger()->info(__VA_ARGS__)
#define IGE_CORE_WARN(...) ::iGe::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define IGE_CORE_ERROR(...) ::iGe::Log::GetCoreLogger()->error(__VA_ARGS__)
#define IGE_CORE_CRITICAL(...) ::iGe::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define IGE_TRACE(...) ::iGe::Log::GetClientLogger()->trace(__VA_ARGS__)
#define IGE_DEBUG(...) ::iGe::Log::GetClientLogger()->debug(__VA_ARGS__)
#define IGE_INFO(...) ::iGe::Log::GetClientLogger()->info(__VA_ARGS__)
#define IGE_WARN(...) ::iGe::Log::GetClientLogger()->warn(__VA_ARGS__)
#define IGE_ERROR(...) ::iGe::Log::GetClientLogger()->error(__VA_ARGS__)
#define IGE_CRITICAL(...) ::iGe::Log::GetClientLogger()->critical(__VA_ARGS__)
