module iGe.Core;
import :Application;
import iGe.Renderer;

namespace iGe
{

// =================================================================================================
// Application
// =================================================================================================

Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& specification) : m_Specification{specification} {
    Internal::Assert(!s_Instance, "Application already exists!");
    s_Instance = this;

    m_Window = Window::Create();
    m_Window->SetEventCallback(std::bind(&Application::OnEvent, this, std::placeholders::_1));

    // Initialize RHI if not already initialized
    if (!RHI::Get()) {
        RHI::Config config;
        config.GraphicsAPI = GraphicsAPI::DirectX12;
        RHI::Init(config);
    }

    if (!RHIImGuiContext::Get()) {
        RHIImGuiContext::Config config;
        config.Window = m_Window->GetNativeWindow();
        config.MaxFramesInFlight = MAX_FRAMES_IN_FLIGHT;
        RHIImGuiContext::Init(config);
    }

    CreateSwapChain();
    CreateCommandPool();
    CreateInFlightResouce();
}

Application::~Application() {}

void Application::Run() {
    while (m_Running) {
        m_CurrentFrame = m_SwapChain->AcquireNextImage();

        // Sync: Wait for previous frame with same index to finish
        m_InFlightFences[m_CurrentFrame]->Wait();
        m_InFlightFences[m_CurrentFrame]->Reset();

        static auto startTime = std::chrono::high_resolution_clock::now();
        auto currentTime = std::chrono::high_resolution_clock::now();
        float32 time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        Timestep timestep{time - m_LastTime};
        m_LastTime = time;

        // Layer rendering
        for (auto layer: m_LayerStack.layers()) { layer->OnUpdate(timestep); }

        // ImGui rendering
        auto backBufferTexture = m_SwapChain->GetBackBufferTexture(m_CurrentFrame);
        {
            // Transition back buffer to Present for ImGui
            auto cmdList = RHI::Get()->AllocateCommandList(m_CommandPool.Get());
            cmdList->Reset();
            cmdList->Begin();
            cmdList->ResourceBarrier(backBufferTexture, RHILayout::Undefined, RHILayout::Present);
            cmdList->End();
            RHI::Get()->GetQueue(RHIQueueType::Graphics)->Submit(cmdList.get());

            // Render ImGui
            RHIImGuiContext::Get()->Begin(m_CurrentFrame);
            RHIImGuiContext::Get()->SetRenderTarget(*backBufferTexture);
            for (auto layer: m_LayerStack.layers()) { layer->OnImGuiRender(); }
            RHIImGuiContext::Get()->End();
        }

        // Submit dummy command list to signal fence and semaphore
        // We wait for ImageAvailable, and Signal RenderFinished
        auto& cmdList = m_CommandLists[m_CurrentFrame];
        cmdList->Reset();
        cmdList->Begin();
        cmdList->End();

        // std::array<RHISemaphore*, 1> waitSems = {m_ImageAvailableSemaphores[m_CurrentFrame].get()};
        std::array<RHISemaphore*, 1> signalSems = {m_RenderFinishedSemaphores[m_CurrentFrame].get()};
        RHI::Get()
                ->GetQueue(RHIQueueType::Graphics)
                ->Submit(cmdList.get(), m_InFlightFences[m_CurrentFrame].get(), {}, signalSems);

        std::array<RHISemaphore*, 1> presentWaitSemaphores = {m_RenderFinishedSemaphores[m_CurrentFrame].get()};
        m_SwapChain->Present(presentWaitSemaphores);
        m_Window->OnUpdate();
    }

    if (auto rhi = RHI::Get()) { rhi->WaitIdle(); }
}

RHITexture* Application::GetCurrentBackBufferTexture() const {
    return m_SwapChain->GetBackBufferTexture(m_CurrentFrame);
}

RHITextureView* Application::GetCurrentBackBufferView() const { return m_SwapChain->GetBackBufferView(m_CurrentFrame); }

void Application::OnEvent(Event& e) {
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<WindowResizeEvent>(std::bind(&Application::OnWindowResizeEvent, this, std::placeholders::_1));
    dispatcher.Dispatch<WindowCloseEvent>(std::bind(&Application::OnWindowCloseEvent, this, std::placeholders::_1));

    for (auto layer: m_LayerStack.layers() | std::views::reverse) {
        if (e.m_Handled) { break; }
        layer->OnEvent(e);
    }
}

bool Application::OnWindowResizeEvent(WindowResizeEvent& event) {
    RHI::Get()->WaitIdle();
    m_SwapChain->Resize(event.GetWidth(), event.GetHeight());
    return false;
}

bool Application::OnWindowCloseEvent(WindowCloseEvent& event) {
    m_Running = false;
    return true;
}

void Application::CreateSwapChain() {
    auto rhi = RHI::Get();

    RHISurfaceCreateInfo surfaceInfo;
    surfaceInfo.WindowHandle = m_Window->GetNativeWindowHandle();
    m_Surface = rhi->CreateSurface(surfaceInfo);

    RHISwapChainCreateInfo swapChainInfo;
    swapChainInfo.Surface = m_Surface.get();
    swapChainInfo.PresentQueue = rhi->GetQueue(RHIQueueType::Graphics);
    swapChainInfo.Extent = {m_Window->GetWidth(), m_Window->GetHeight()};
    swapChainInfo.Format = RHIFormat::R8G8B8A8UNorm;
    swapChainInfo.ImageCount = MAX_FRAMES_IN_FLIGHT;
    m_SwapChain = rhi->CreateSwapChain(swapChainInfo);
}

void Application::CreateCommandPool() {
    auto rhi = RHI::Get();
    m_CommandPool = rhi->CreateCommandPool({rhi->GetQueue(RHIQueueType::Graphics)});
}

void Application::CreateInFlightResouce() {
    auto rhi = RHI::Get();

    // Initialize per-frame resources
    m_CommandLists.resize(MAX_FRAMES_IN_FLIGHT);
    m_InFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_ImageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_RenderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

    for (uint32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_CommandLists[i] = RHI::Get()->AllocateCommandList(m_CommandPool.Get());
        m_InFlightFences[i] = rhi->CreateGPUFence({true, 0}); // Create signaled for first frame
        m_ImageAvailableSemaphores[i] = rhi->CreateGPUSemaphore();
        m_RenderFinishedSemaphores[i] = rhi->CreateGPUSemaphore();
    }
}

void Application::PushLayer(Ref<Layer> layer) {
    m_LayerStack.PushLayer(layer);
    layer->OnAttach();
}

void Application::PushOverlay(Ref<Layer> layer) {
    m_LayerStack.PushOverlay(layer);
    layer->OnAttach();
}

} // namespace iGe
