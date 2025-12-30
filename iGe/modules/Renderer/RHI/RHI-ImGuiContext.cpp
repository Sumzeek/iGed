module iGe.RHI;
import :RHIImGuiContext;
import :RHI;

#if defined(IGE_PLATFORM_WINDOWS)
import :DirectX12ImGuiContext;
#endif

namespace iGe
{

// =================================================================================================
// RHIImGuiContext
// =================================================================================================

RHIImGuiContext* RHIImGuiContext::Init(const Config& config) {
    s_Config = config;

    switch (RHI::Get()->GetGraphicsAPI()) {
        case GraphicsAPI::Vulkan:
            // TODO: Implement VulkanRHI
            break;

#if defined(IGE_PLATFORM_WINDOWS)
        case GraphicsAPI::DirectX12:
            s_ImGuiContext = CreateScope<DirectX12ImGuiContext>();
            break;
#endif

        case GraphicsAPI::Metal:
            // TODO: Implement MetalRHI
            break;
        default:
            Internal::LogError("Unknown GraphicsAPI");
            break;
    }

    return s_ImGuiContext.get();
}

} // namespace iGe
