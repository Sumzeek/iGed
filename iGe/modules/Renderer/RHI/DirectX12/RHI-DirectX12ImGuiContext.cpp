module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include "backends/imgui_impl_dx12.h"
    #include "backends/imgui_impl_glfw.h"
    #include <d3d12.h>
    #include <dxgi1_4.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12RHI;
import :DirectX12ImGuiContext;
import :DirectX12Texture;
import :DirectX12Queue;

namespace iGe
{

// =================================================================================================
// DirectX12ImGuiContext
// =================================================================================================

DirectX12ImGuiContext::DirectX12ImGuiContext() {
    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   // Enable Docking
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // Enable Multi-Viewport / Platform Windows

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // when viewports are enabled we tweak WindowRounding/windowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    GLFWwindow* window = static_cast<GLFWwindow*>(s_Config.Window);

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOther(window, true);

    // Initialize DX12
    auto* dx12RHI = static_cast<DirectX12RHI*>(RHI::Get());
    auto device = dx12RHI->GetD3D12Device();

    // Create Command Pool
    D3D12_COMMAND_LIST_TYPE type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    if (FAILED(device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_Allocator)))) {
        Internal::LogError("Failed to create command allocator");
    }

    // Create Command Lists
    uint32 count = s_Config.MaxFramesInFlight;

    m_CommandLists.resize(count);
    for (uint32 i = 0; i < count; ++i) {
        if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_Allocator.Get(), nullptr,
                                             IID_PPV_ARGS(&m_CommandLists[i])))) {
            Internal::LogError("Failed to create command list");
        }
        m_CommandLists[i]->Close();
    }

    // Create SRV Descriptor Heap
    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = 1;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    if (FAILED(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_SrvDescHeap)))) {
        Internal::LogError("Failed to create SRV Descriptor Heap for ImGui");
        return;
    }

    // Init ImGui DX12
    // Assuming 3 frames in flight and R8G8B8A8_UNORM format as defaults
    ImGui_ImplDX12_Init(device, 3, DXGI_FORMAT_R8G8B8A8_UNORM, m_SrvDescHeap.Get(),
                        m_SrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
                        m_SrvDescHeap->GetGPUDescriptorHandleForHeapStart());
}

DirectX12ImGuiContext::~DirectX12ImGuiContext() {
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_CommandLists.clear();
    m_SrvDescHeap.Reset();
}

void DirectX12ImGuiContext::Begin(uint32 frameIndex) {
    m_FrameIndex = frameIndex;
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void DirectX12ImGuiContext::End() {
    ImGui::Render();

    if (!m_RenderTarget) {
        Internal::LogError("No Render Target set for ImGui Context!");
        return;
    }

    auto dxTexture = static_cast<DirectX12Texture*>(m_RenderTarget);
    auto pBackBuffer = dxTexture->GetResource();
    auto rtvHandle = dxTexture->GetRTV();

    // Reset allocator & command list
    auto& commandList = m_CommandLists[m_FrameIndex];
    commandList->Reset(m_Allocator.Get(), nullptr);

    // Transition Layout to RenderTarget
    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = pBackBuffer;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
    commandList->ResourceBarrier(1, &barrier);

    // Set Render Target
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Set descriptor heap
    ID3D12DescriptorHeap* heaps[] = {m_SrvDescHeap.Get()};
    commandList->SetDescriptorHeaps(1, heaps);

    // Render ImGui draw data
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());

    // Multi-viewport support
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault(nullptr, (void*) commandList.Get());
    }

    // Transition Layout to Present
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    commandList->ResourceBarrier(1, &barrier);

    // Close command list
    commandList->Close();

    // Execute command list
    auto* dx12RHI = static_cast<DirectX12RHI*>(RHI::Get());
    auto* rhiQueue = dx12RHI->GetQueue(RHIQueueType::Graphics);
    auto* commandQueue = static_cast<ID3D12CommandQueue*>(rhiQueue->GetNativeHandle());

    ID3D12CommandList* lists[] = {commandList.Get()};
    commandQueue->ExecuteCommandLists(1, lists);
}

} // namespace iGe
#endif
