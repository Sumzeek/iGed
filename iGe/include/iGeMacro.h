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
#define IGE_CORE_TRACE(...) ::iGe::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define IGE_CORE_INFO(...) ::iGe::Log::GetCoreLogger()->info(__VA_ARGS__)
#define IGE_CORE_WARN(...) ::iGe::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define IGE_CORE_ERROR(...) ::iGe::Log::GetCoreLogger()->error(__VA_ARGS__)
#define IGE_CORE_CRITICAL(...) ::iGe::Log::GetCoreLogger()->critical(__VA_ARGS__)
// Client log macros
#define IGE_TRACE(...) ::iGe::Log::GetClientLogger()->trace(__VA_ARGS__)
#define IGE_INFO(...) ::iGe::Log::GetClientLogger()->info(__VA_ARGS__)
#define IGE_WARN(...) ::iGe::Log::GetClientLogger()->warn(__VA_ARGS__)
#define IGE_ERROR(...) ::iGe::Log::GetClientLogger()->error(__VA_ARGS__)
#define IGE_CRITICAL(...) ::iGe::Log::GetClientLogger()->critical(__VA_ARGS__)

// This block defines assertion macros that are only enabled when IGE_ENABLE_ASSERTS is defined.
// The assertions will log an error message and break into the debugger if the condition is false.
#ifdef IGE_ENABLE_ASSERTS
    #define IGE_ASSERT(x, message)                                                                                     \
        {                                                                                                              \
            if (!(x)) {                                                                                                \
                IGE_ERROR("Assertion Failed: {0}", message);                                                           \
                __debugbreak();                                                                                        \
            }                                                                                                          \
        }
    #define IGE_CORE_ASSERT(x, message)                                                                                \
        {                                                                                                              \
            if (!(x)) {                                                                                                \
                IGE_CORE_ERROR("Assertion Failed: {0}", message);                                                      \
                __debugbreak();                                                                                        \
            }                                                                                                          \
        }
#else
    #define IGE_ASSERT(x, ...)
    #define IGE_CORE_ASSERT(x, ...)
#endif

// Define BIT macro to create a bitmask by shifting 1 to the left by x positions
#define BIT(x) (1 << x)

// Define IGE_BIND_EVENT_FN to bind member functions for event handling
#define IGE_BIND_EVENT_FN(fn) std::bind(&fn, this, std::placeholders::_1)