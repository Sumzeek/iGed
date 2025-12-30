module iGe.Core;
import :Input;

#if defined(IGE_PLATFORM_WINDOWS)
import :WindowsInput;
#endif

namespace iGe
{

// =================================================================================================
// Input
// =================================================================================================

#if defined(IGE_PLATFORM_WINDOWS)
Input* Input::s_Instance = new WindowsInput{};
#elif defined(IGE_PLATFORM_LINUX)
    #error "Unsupported platform!"
#elif defined(IGE_PLATFORM_MACOS)
    #error "Unsupported platform!"
#else
    #error "Unsupported platform!"
#endif

} // namespace iGe
