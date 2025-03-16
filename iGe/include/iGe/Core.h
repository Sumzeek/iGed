#pragma once

#ifdef IGE_EXPORT
    #define IGE_API __declspec(dllexport)
#else
    #define IGE_API __declspec(dllimport)
#endif

#define BIT(x) (1 << x)
