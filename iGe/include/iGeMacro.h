#pragma once

// This block handles the declaration of the IGE_API macro based on platform and export/import settings.
// It helps in defining the correct linkage for Windows DLLs.
#ifdef IGE_PLATFORM_WINDOWS
    #ifdef IGE_EXPORT
        #define IGE_API __declspec(dllexport)
    #else
        #define IGE_API __declspec(dllimport)
    #endif
#else
    #error iGe only supports Windows!
#endif

// Core log macros
#define IGE_CORE_TRACE(...) ::iGe::Log::GetCoreLogger()->trace(std::format(__VA_ARGS__))
#define IGE_CORE_INFO(...) ::iGe::Log::GetCoreLogger()->info(std::format(__VA_ARGS__))
#define IGE_CORE_WARN(...) ::iGe::Log::GetCoreLogger()->warn(std::format(__VA_ARGS__))
#define IGE_CORE_ERROR(...) ::iGe::Log::GetCoreLogger()->error(std::format(__VA_ARGS__))
#define IGE_CORE_CRITICAL(...) ::iGe::Log::GetCoreLogger()->critical(std::format(__VA_ARGS__))

// Client log macros
#define IGE_TRACE(...) ::iGe::Log::GetClientLogger()->trace(std::format(__VA_ARGS__))
#define IGE_INFO(...) ::iGe::Log::GetClientLogger()->info(std::format(__VA_ARGS__))
#define IGE_WARN(...) ::iGe::Log::GetClientLogger()->warn(std::format(__VA_ARGS__))
#define IGE_ERROR(...) ::iGe::Log::GetClientLogger()->error(std::format(__VA_ARGS__))
#define IGE_CRITICAL(...) ::iGe::Log::GetClientLogger()->critical(std::format(__VA_ARGS__))

#define IGE_EXPAND_MACRO(x) x
#define IGE_STRINGIFY_MACRO(x) #x

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

#ifdef IGE_ENABLE_ASSERTS
    #define IGE_INTERNAL_ASSERT_IMPL(type, check, msg, ...)                                                            \
        do {                                                                                                           \
            if (!(check)) {                                                                                            \
                IGE##type##ERROR(msg, __VA_ARGS__);                                                                    \
                IGE_DEBUGBREAK();                                                                                      \
            }                                                                                                          \
        } while (0)

    #define IGE_INTERNAL_ASSERT_WITH_MSG(type, check, ...)                                                             \
        IGE_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", __VA_ARGS__)
    #define IGE_INTERNAL_ASSERT_NO_MSG(type, check)                                                                    \
        IGE_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", #check,                             \
                                 std::filesystem::path(__FILE__).filename().string(), __LINE__)

    // Currently accepts at least the condition and one additional parameter (the message) being optional
    #define IGE_INTERNAL_ASSERT_GET_MACRO(_1, _2, NAME, ...) NAME
    #define IGE_ASSERT(...)                                                                                            \
        IGE_EXPAND_MACRO(IGE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__, IGE_INTERNAL_ASSERT_WITH_MSG,                      \
                                                       IGE_INTERNAL_ASSERT_NO_MSG)(_, __VA_ARGS__))
    #define IGE_CORE_ASSERT(...)                                                                                       \
        IGE_EXPAND_MACRO(IGE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__, IGE_INTERNAL_ASSERT_WITH_MSG,                      \
                                                       IGE_INTERNAL_ASSERT_NO_MSG)(_CORE_, __VA_ARGS__))
#else
    #define IGE_ASSERT(...)
    #define IGE_CORE_ASSERT(...)
#endif

// Define BIT macro to create a bitmask by shifting 1 to the left by x positions
#define BIT(x) (1 << x)

// Define IGE_BIND_EVENT_FN to bind member functions for event handling
#define IGE_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)
