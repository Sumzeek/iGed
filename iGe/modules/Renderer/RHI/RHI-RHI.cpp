module iGe.RHI;
import :RHI;

#if defined(IGE_PLATFORM_WINDOWS)
import :DirectX12RHI;
#endif

namespace iGe
{

// =================================================================================================
// RHI
// =================================================================================================

RHI* RHI::Init(const Config& config) {
    s_Config = config;

    switch (s_Config.GraphicsAPI) {
        case GraphicsAPI::Vulkan:
            // TODO: Implement VulkanRHI
            break;

#if defined(IGE_PLATFORM_WINDOWS)
        case GraphicsAPI::DirectX12:
            s_RHI = CreateScope<DirectX12RHI>();
            break;
#endif

        case GraphicsAPI::Metal:
            // TODO: Implement MetalRHI
            break;
        default:
            Internal::LogError("Unknown GraphicsAPI");
            break;
    }

    return s_RHI.get();
}

} // namespace iGe
