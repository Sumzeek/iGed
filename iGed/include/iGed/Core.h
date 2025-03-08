#pragma once

#ifdef IGED_EXPORT
    #define IGED_API __declspec(dllexport)
#else
    #define IGED_API __declspec(dllimport)
#endif
