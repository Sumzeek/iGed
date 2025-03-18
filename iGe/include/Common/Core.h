#pragma once

#ifdef IGE_PLATFORM_WINDOWS
    #ifdef IGE_EXPORT
        #define IGE_API __declspec(dllexport)
    #else
        #define IGE_API __declspec(dllimport)
    #endif
#else
    #error iGe only supports Window!
#endif

#ifdef IGE_ENABLE_ASSERTS
    #define IGE_ASSERT(x, ...)                                                                                         \
        {                                                                                                              \
            if (!(x)) {                                                                                                \
                IGE_ERROR("Assertion Failed: {0}", __VA__ARGS__);                                                      \
                __debugbreak();                                                                                        \
            }                                                                                                          \
        }
    #define IGE_CORE_ASSERT(x, ...)                                                                                    \
        {                                                                                                              \
            if (!(x)) {                                                                                                \
                IGE_CORE_ERROR("Assertion Failed: {0}", __VA__ARGS__);                                                 \
                __debugbreak();                                                                                        \
            }                                                                                                          \
        }
#else
    #define IGE_ASSERT(x, ...)
    #define IGE_CORE_ASSERT(x, ...)
#endif

#define BIT(x) (1 << x)
