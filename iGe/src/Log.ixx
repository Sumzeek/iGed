#include "Common/Core.h"
#include "spdlog/fmt/ostr.h" // enable << support
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

export module iGe.Log;

namespace iGe
{

export class IGE_API Log {
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

std::shared_ptr<spdlog::logger> Log::m_CoreLogger = nullptr;
std::shared_ptr<spdlog::logger> Log::m_ClientLogger = nullptr;

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

export inline IGE_API void CoreTrace(const auto&... args) { Log::GetCoreLogger()->trace(args...); }
export inline IGE_API void CoreDebug(const auto&... args) { Log::GetCoreLogger()->debug(args...); }
export inline IGE_API void CoreInfo(const auto&... args) { Log::GetCoreLogger()->info(args...); }
export inline IGE_API void CoreWarn(const auto&... args) { Log::GetCoreLogger()->warn(args...); }
export inline IGE_API void CoreError(const auto&... args) { Log::GetCoreLogger()->error(args...); }
export inline IGE_API void CoreCritical(const auto&... args) { Log::GetCoreLogger()->critical(args...); }

export inline IGE_API void ClientTrace(const auto&... args) { Log::GetClientLogger()->trace(args...); }
export inline IGE_API void ClientDebug(const auto&... args) { Log::GetClientLogger()->debug(args...); }
export inline IGE_API void ClientInfo(const auto&... args) { Log::GetClientLogger()->info(args...); }
export inline IGE_API void ClientWarn(const auto&... args) { Log::GetClientLogger()->warn(args...); }
export inline IGE_API void ClientError(const auto&... args) { Log::GetClientLogger()->error(args...); }
export inline IGE_API void ClientCritical(const auto&... args) { Log::GetClientLogger()->critical(args...); }

} // namespace iGe
