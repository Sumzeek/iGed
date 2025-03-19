module;
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

    static inline void CoreTrace(std::string_view s);
    static inline void CoreDebug(std::string_view s);
    static inline void CoreInfo(std::string_view s);
    static inline void CoreWarn(std::string_view s);
    static inline void CoreError(std::string_view s);
    static inline void CoreCritical(std::string_view s);

    static inline void ClientTrace(std::string_view s);
    static inline void ClientDebug(std::string_view s);
    static inline void ClientInfo(std::string_view s);
    static inline void ClientWarn(std::string_view s);
    static inline void ClientError(std::string_view s);
    static inline void ClientCritical(std::string_view s);

    //template<typename... Args>
    //static void CoreTrace(std::format_string<Args...> fmt, Args&&... args);
    //template<typename... Args>
    //static void CoreDebug(std::format_string<Args...> fmt, Args&&... args);
    //template<typename... Args>
    //static void CoreInfo(std::format_string<Args...> fmt, Args&&... args);
    //template<typename... Args>
    //static void CoreWarn(std::format_string<Args...> fmt, Args&&... args);
    //template<typename... Args>
    //static void CoreError(std::format_string<Args...> fmt, Args&&... args);
    //template<typename... Args>
    //static void CoreCritical(std::format_string<Args...> fmt, Args&&... args);
    //
    //template<typename... Args>
    //static void ClientTrace(std::format_string<Args...> fmt, Args&&... args);
    //template<typename... Args>
    //static void ClientDebug(std::format_string<Args...> fmt, Args&&... args);
    //template<typename... Args>
    //static void ClientInfo(std::format_string<Args...> fmt, Args&&... args);
    //template<typename... Args>
    //static void ClientWarn(std::format_string<Args...> fmt, Args&&... args);
    //template<typename... Args>
    //static void ClientError(std::format_string<Args...> fmt, Args&&... args);
    //template<typename... Args>
    //static void ClientCritical(std::format_string<Args...> fmt, Args&&... args);

private:
    Log();
    ~Log();

    static std::shared_ptr<spdlog::logger>& GetCoreLogger();
    static std::shared_ptr<spdlog::logger>& GetClientLogger();

    static std::shared_ptr<spdlog::logger> m_CoreLogger;
    static std::shared_ptr<spdlog::logger> m_ClientLogger;
};

// ----------------- Log::Implementation -----------------
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

void Log::CoreTrace(std::string_view s) { Log::GetCoreLogger()->trace(s); }
void Log::CoreDebug(std::string_view s) { Log::GetCoreLogger()->debug(s); }
void Log::CoreInfo(std::string_view s) { Log::GetCoreLogger()->info(s); }
void Log::CoreWarn(std::string_view s) { Log::GetCoreLogger()->warn(s); }
void Log::CoreError(std::string_view s) { Log::GetCoreLogger()->error(s); }
void Log::CoreCritical(std::string_view s) { Log::GetCoreLogger()->critical(s); }

void Log::ClientTrace(std::string_view s) { Log::GetClientLogger()->trace(s); }
void Log::ClientDebug(std::string_view s) { Log::GetClientLogger()->debug(s); }
void Log::ClientInfo(std::string_view s) { Log::GetClientLogger()->info(s); }
void Log::ClientWarn(std::string_view s) { Log::GetClientLogger()->warn(s); }
void Log::ClientError(std::string_view s) { Log::GetClientLogger()->error(s); }
void Log::ClientCritical(std::string_view s) { Log::GetClientLogger()->critical(s); }

//template<typename... Args>
//void Log::CoreTrace(std::format_string<Args...> fmt, Args&&... args) {
//    Log::GetCoreLogger()->trace(std::format(fmt, std::forward<Args>(args)...));
//}
//template<typename... Args>
//void Log::CoreDebug(std::format_string<Args...> fmt, Args&&... args) {
//    Log::GetCoreLogger()->debug(std::format(fmt, std::forward<Args>(args)...));
//}
//template<typename... Args>
//void Log::CoreInfo(std::format_string<Args...> fmt, Args&&... args) {
//    Log::GetCoreLogger()->info(std::format(fmt, std::forward<Args>(args)...));
//}
//template<typename... Args>
//void Log::CoreWarn(std::format_string<Args...> fmt, Args&&... args) {
//    Log::GetCoreLogger()->warn(std::format(fmt, std::forward<Args>(args)...));
//}
//template<typename... Args>
//void Log::CoreError(std::format_string<Args...> fmt, Args&&... args) {
//    Log::GetCoreLogger()->error(std::format(fmt, std::forward<Args>(args)...));
//}
//template<typename... Args>
//void Log::CoreCritical(std::format_string<Args...> fmt, Args&&... args) {
//    Log::GetCoreLogger()->critical(std::format(fmt, std::forward<Args>(args)...));
//}
//
//template<typename... Args>
//void Log::ClientTrace(std::format_string<Args...> fmt, Args&&... args) {
//    Log::GetClientLogger()->trace(std::format(fmt, std::forward<Args>(args)...));
//}
//template<typename... Args>
//void Log::ClientDebug(std::format_string<Args...> fmt, Args&&... args) {
//    Log::GetClientLogger()->debug(std::format(fmt, std::forward<Args>(args)...));
//}
//template<typename... Args>
//void Log::ClientInfo(std::format_string<Args...> fmt, Args&&... args) {
//    Log::GetClientLogger()->info(std::format(fmt, std::forward<Args>(args)...));
//}
//template<typename... Args>
//void Log::ClientWarn(std::format_string<Args...> fmt, Args&&... args) {
//    Log::GetClientLogger()->warn(std::format(fmt, std::forward<Args>(args)...));
//}
//template<typename... Args>
//void Log::ClientError(std::format_string<Args...> fmt, Args&&... args) {
//    Log::GetClientLogger()->error(std::format(fmt, std::forward<Args>(args)...));
//}
//template<typename... Args>
//void Log::ClientCritical(std::format_string<Args...> fmt, Args&&... args) {
//    Log::GetClientLogger()->critical(std::format(fmt, std::forward<Args>(args)...));
//}

} // namespace iGe
