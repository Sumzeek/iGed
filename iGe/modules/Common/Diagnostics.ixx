module;
#include "iGeMacro.h"

export module iGe.Diagnostics;
import iGe.Types;
import iGe.Log;

#ifdef IGE_DEBUG
    #ifdef IGE_PLATFORM_WINDOWS
        #define IGE_DEBUGBREAK() __debugbreak()
    #elif IGE_PLATFORM_LINUX
        #define IGE_DEBUGBREAK() raise(SIGTRAP)
    #else
        #error "Platform doesn't support debugbreak yet!"
    #endif
    #define IGE_ENABLE_ASSERTS
#else
    #define IGE_DEBUGBREAK()
#endif

namespace iGe
{
export template<typename... Args>
inline void LogTrace(std::format_string<Args...> fmt, Args&&... args) {
    Log::GetClientLogger()->trace(std::format(fmt, std::forward<Args>(args)...));
}
export template<typename... Args>
inline void LogInfo(std::format_string<Args...> fmt, Args&&... args) {
    Log::GetClientLogger()->info(std::format(fmt, std::forward<Args>(args)...));
}
export template<typename... Args>
inline void LogWarn(std::format_string<Args...> fmt, Args&&... args) {
    Log::GetClientLogger()->warn(std::format(fmt, std::forward<Args>(args)...));
}
export template<typename... Args>
inline void LogError(std::format_string<Args...> fmt, Args&&... args) {
    Log::GetClientLogger()->error(std::format(fmt, std::forward<Args>(args)...));
}
export template<typename... Args>
inline void LogCritical(std::format_string<Args...> fmt, Args&&... args) {
    Log::GetClientLogger()->critical(std::format(fmt, std::forward<Args>(args)...));
}

export inline void Assert(bool condition, const std::source_location& loc = std::source_location::current()) {
#ifdef IGE_ENABLE_ASSERTS
    if (condition) { return; }
    LogError("Assertion failed at {}:{}", loc.file_name(), loc.line());
    IGE_DEBUGBREAK();
#endif
}

export template<typename... Args>
inline void Assert(bool condition, std::string msg, const std::source_location& loc = std::source_location::current()) {
#ifdef IGE_ENABLE_ASSERTS
    if (condition) { return; }
    LogError("Assertion failed: {}, at {}:{}", msg, loc.file_name(), loc.line());
    IGE_DEBUGBREAK();
#endif
}
} // namespace iGe

namespace iGe::Internal
{
export template<typename... Args>
inline void LogTrace(std::format_string<Args...> fmt, Args&&... args) {
    Log::GetCoreLogger()->trace(std::format(fmt, std::forward<Args>(args)...));
}
export template<typename... Args>
inline void LogInfo(std::format_string<Args...> fmt, Args&&... args) {
    Log::GetCoreLogger()->info(std::format(fmt, std::forward<Args>(args)...));
}
export template<typename... Args>
inline void LogWarn(std::format_string<Args...> fmt, Args&&... args) {
    Log::GetCoreLogger()->warn(std::format(fmt, std::forward<Args>(args)...));
}
export template<typename... Args>
inline void LogError(std::format_string<Args...> fmt, Args&&... args) {
    Log::GetCoreLogger()->error(std::format(fmt, std::forward<Args>(args)...));
}
export template<typename... Args>
inline void LogCritical(std::format_string<Args...> fmt, Args&&... args) {
    Log::GetCoreLogger()->critical(std::format(fmt, std::forward<Args>(args)...));
}

export inline void Assert(bool condition, const std::source_location& loc = std::source_location::current()) {
#ifdef IGE_ENABLE_ASSERTS
    if (condition) { return; }
    LogError("Assertion failed at {}:{}", loc.file_name(), loc.line());
    IGE_DEBUGBREAK();
#endif
}

export template<typename... Args>
inline void Assert(bool condition, std::string msg, const std::source_location& loc = std::source_location::current()) {
#ifdef IGE_ENABLE_ASSERTS
    if (condition) { return; }
    LogError("Assertion failed: {}, at {}:{}", msg, loc.file_name(), loc.line());
    IGE_DEBUGBREAK();
#endif
}
} // namespace iGe::Internal
