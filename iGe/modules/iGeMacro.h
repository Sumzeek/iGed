#pragma once

// This block handles the declaration of the IGE_API macro based on platform and export/import settings.
// It helps in defining the correct linkage for Windows DLLs.
#if defined(IGE_PLATFORM_WINDOWS)
    #define IGE_API __declspec(dllexport)
#elif defined(IGE_PLATFORM_MACOS) || defined(IGE_PLATFORM_LINUX)
    #define IGE_API __attribute__((visibility("default")))
#else
    #error "Unsupported platform!"
#endif
