module;
#include "imgui.h"

module iGed.ExampleLayer;

import iGe;
import iGe.RHI;
import iGe.Renderer;

// =================================================================================================
// ExampleLayer
// =================================================================================================

ExampleLayer::ExampleLayer() : Layer{"Example"}, m_Camera{-1.6f, 1.6f, -0.9f, 0.9f}, m_CameraPosition{0.0f} {
    CreateCommandPool();
    CreateRenderPass();
    CreatePipelineLayout(); // Creates DescriptorSetLayout and PipelineLayout
    CreateGraphicsPipeline();
    CreateBuffers();             // Creates textures, buffers, texture views
    CreateDescriptorResources(); // Creates sampler, pool, descriptor set (needs m_TextureView, m_UniformBuffer)

    m_CommandList = iGe::RHI::Get()->AllocateCommandList(m_CommandPool.Get());

    // Create depth texture and view
    CreateDepthResources(iGe::Application::Get().GetWindow().GetWidth(),
                         iGe::Application::Get().GetWindow().GetHeight());
}

void ExampleLayer::OnUpdate(iGe::Timestep ts) {
    // Camera movement
    if (iGe::Input::IsKeyPressed(iGeKey::W)) {
        m_CameraPosition.y -= m_CameraMoveSpeed * ts;
    } else if (iGe::Input::IsKeyPressed(iGeKey::S)) {
        m_CameraPosition.y += m_CameraMoveSpeed * ts;
    }

    if (iGe::Input::IsKeyPressed(iGeKey::A)) {
        m_CameraPosition.x += m_CameraMoveSpeed * ts;
    } else if (iGe::Input::IsKeyPressed(iGeKey::D)) {
        m_CameraPosition.x -= m_CameraMoveSpeed * ts;
    }

    if (iGe::Input::IsKeyPressed(iGeKey::Q)) {
        m_CameraRotation -= m_CameraRotationSpeed * ts;
    } else if (iGe::Input::IsKeyPressed(iGeKey::E)) {
        m_CameraRotation += m_CameraRotationSpeed * ts;
    }

    m_Camera.SetPosition(m_CameraPosition);
    m_Camera.SetRotation(m_CameraRotation);

    // Calculate model transform
    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    glm::mat4 model = glm::gtc::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Update Uniform Buffer
    if (void* ubPtr = m_UniformBuffer->Map()) {
        const auto& viewProj = m_Camera.GetViewProjectionMatrix();
        memcpy(ubPtr, &viewProj, sizeof(glm::mat4));
        memcpy(static_cast<uint8*>(ubPtr) + sizeof(glm::mat4), &model, sizeof(glm::mat4));
        m_UniformBuffer->Unmap();
    }

    // Rendering
    auto queue = iGe::RHI::Get()->GetQueue(iGe::RHIQueueType::Graphics);
    uint32 width = iGe::Application::Get().GetWindow().GetWidth();
    uint32 height = iGe::Application::Get().GetWindow().GetHeight();

    // Get back buffer texture and view from SwapChain
    auto colorTexture = iGe::Application::Get().GetCurrentBackBufferTexture();
    auto colorTextureView = iGe::Application::Get().GetCurrentBackBufferView();

    // Reset command list for new frame
    m_CommandList->Reset();
    m_CommandList->Begin();
    {
        // Set viewport
        iGe::RHIViewport viewport{};
        viewport.X = 0;
        viewport.Y = 0;
        viewport.Width = static_cast<float>(width);
        viewport.Height = static_cast<float>(height);
        viewport.MinDepth = 0.0f;
        viewport.MaxDepth = 1.0f;
        m_CommandList->SetViewport(viewport);

        // Set scissor
        iGe::RHIScissor scissor{};
        scissor.X = 0;
        scissor.Y = 0;
        scissor.Width = static_cast<float>(width);
        scissor.Height = static_cast<float>(height);
        m_CommandList->SetScissor(scissor);

        // Transition color texture: Present -> ColorAttachment
        // (Depth buffer is already in DepthStencilAttachment state since creation)
        m_CommandList->ResourceBarrier(colorTexture, iGe::RHILayout::Present, iGe::RHILayout::ColorAttachment);

        // Prepare attachment bindings
        iGe::RHIAttachmentBinding colorBinding{};
        colorBinding.pTextureView = colorTextureView;
        colorBinding.ClearValue = iGe::RHIClearValue::CreateColor(0.5f, 0.5f, 0.5f, 1.0f);

        iGe::RHIAttachmentBinding depthBinding{};
        depthBinding.pTextureView = m_DepthTextureView.get();
        depthBinding.ClearValue = iGe::RHIClearValue::CreateDepthStencil(1.0f, 0);

        // Set up render pass begin info
        iGe::RHIRenderPassBeginInfo beginInfo{};
        beginInfo.pRenderPass = m_RenderPass.get();
        beginInfo.ColorAttachments = {&colorBinding, 1};
        beginInfo.pDepthStencilAttachment = &depthBinding;
        beginInfo.RenderAreaOffset = {0, 0};
        beginInfo.RenderAreaExtent = {width, height};

        m_CommandList->BeginRenderPass(beginInfo);

        // Draw Triangle using Descriptor Set (UBO only)
        m_CommandList->BindGraphicsPipeline(m_TriGraphicsPipeline.get());
        m_CommandList->BindVertexBuffer(m_TriVertexBuffer.get());
        m_CommandList->BindIndexBuffer(m_TriIndexBuffer.get());
        m_CommandList->BindDescriptorSet(m_TriPipelineLayout.get(), 0, m_TriDescriptorSet.get());
        m_CommandList->DrawIndexed(3, 1, 0, 0, 0);

        // // Draw Quad with texture using Descriptor Set
        // m_CommandList->BindGraphicsPipeline(m_QuadGraphicsPipeline.get());
        // m_CommandList->BindVertexBuffer(m_QuadVertexBuffer.get());
        // m_CommandList->BindIndexBuffer(m_QuadIndexBuffer.get());
        // m_CommandList->BindDescriptorSet(m_PipelineLayout.get(), 0, m_DescriptorSet.get());
        // m_CommandList->DrawIndexed(6, 1, 0, 0, 0);

        m_CommandList->EndRenderPass();

        // Transition color attachment to present
        m_CommandList->ResourceBarrier(colorTexture, iGe::RHILayout::ColorAttachment, iGe::RHILayout::Present);
    }
    m_CommandList->End();

    queue->Submit(m_CommandList.get());
}

void ExampleLayer::OnImGuiRender() {
    ImGui::Begin("Settings");
    ImGui::Text("Hello World");
    ImGui::End();
}

void ExampleLayer::OnEvent(iGe::Event& event) {
    iGe::EventDispatcher dispatcher(event);
    dispatcher.Dispatch<iGe::WindowResizeEvent>(
            std::bind(&ExampleLayer::OnWindowResizeEvent, this, std::placeholders::_1));
}

bool ExampleLayer::OnPressedEvent(iGe::KeyPressedEvent& event) {
    if (event.GetKeyCode() == iGeKey::W) { m_CameraPosition.y -= m_CameraMoveSpeed; }
    if (event.GetKeyCode() == iGeKey::A) { m_CameraPosition.x += m_CameraMoveSpeed; }
    if (event.GetKeyCode() == iGeKey::S) { m_CameraPosition.y += m_CameraMoveSpeed; }
    if (event.GetKeyCode() == iGeKey::D) { m_CameraPosition.x -= m_CameraMoveSpeed; }
    if (event.GetKeyCode() == iGeKey::Q) { m_CameraRotation -= m_CameraRotationSpeed; }
    if (event.GetKeyCode() == iGeKey::E) { m_CameraRotation += m_CameraRotationSpeed; }

    return false;
}

bool ExampleLayer::OnWindowResizeEvent(iGe::WindowResizeEvent& event) {
    if (event.GetWidth() == 0 || event.GetHeight() == 0) { return false; }

    // Wait for GPU to finish before resizing resources
    iGe::RHI::Get()->GetQueue(iGe::RHIQueueType::Graphics)->WaitIdle();

    // Recreate depth resources
    CreateDepthResources(event.GetWidth(), event.GetHeight());

    // Update Camera Aspect Ratio
    float aspectRatio = static_cast<float>(event.GetWidth()) / static_cast<float>(event.GetHeight());
    m_Camera.SetProjection(-aspectRatio * 1.6f, aspectRatio * 1.6f, -0.9f, 0.9f);

    return false;
}

void ExampleLayer::CreateCommandPool() {
    iGe::RHICommandPoolCreateInfo poolInfo{};
    poolInfo.pQueue = iGe::RHI::Get()->GetQueue(iGe::RHIQueueType::Graphics);

    m_CommandPool = iGe::RHI::Get()->CreateCommandPool(poolInfo);
}

void ExampleLayer::CreateRenderPass() {
    // Color attachment description
    std::vector<iGe::RHIAttachmentDescription> attachments;

    iGe::RHIAttachmentDescription colorDesc{};
    colorDesc.Format = iGe::RHIFormat::R8G8B8A8UNorm;
    colorDesc.SampleCount = 1;
    colorDesc.LoadOp = iGe::RHILoadOp::Clear;
    colorDesc.StoreOp = iGe::RHIStoreOp::Store;
    colorDesc.StencilLoadOp = iGe::RHILoadOp::DontCare;
    colorDesc.StencilStoreOp = iGe::RHIStoreOp::DontCare;
    colorDesc.InitialLayout = iGe::RHILayout::ColorAttachment;
    colorDesc.FinalLayout = iGe::RHILayout::ColorAttachment;
    attachments.push_back(colorDesc);

    // Depth attachment description
    iGe::RHIAttachmentDescription depthDesc{};
    depthDesc.Format = iGe::RHIFormat::D32SFloat;
    depthDesc.SampleCount = 1;
    depthDesc.LoadOp = iGe::RHILoadOp::Clear;
    depthDesc.StoreOp = iGe::RHIStoreOp::DontCare;
    depthDesc.StencilLoadOp = iGe::RHILoadOp::DontCare;
    depthDesc.StencilStoreOp = iGe::RHIStoreOp::DontCare;
    depthDesc.InitialLayout = iGe::RHILayout::DepthStencilAttachment;
    depthDesc.FinalLayout = iGe::RHILayout::DepthStencilAttachment;
    attachments.push_back(depthDesc);

    // Subpass
    std::vector<uint32> colorAttachmentRefs = {0};
    iGe::RHISubpassDescription subpass{};
    subpass.ColorAttachments = colorAttachmentRefs;
    subpass.DepthStencilAttachment = 1;

    std::vector<iGe::RHISubpassDescription> subpasses = {subpass};

    // Subpass dependency
    iGe::RHISubpassDependency dependency{};
    dependency.SrcSubpass = ~0u; // VK_SUBPASS_EXTERNAL
    dependency.DstSubpass = 0;
    dependency.SrcAccessMask = iGe::RHIDependencyAccess::None;
    dependency.DstAccessMask =
            iGe::RHIDependencyAccess::ColorAttachmentWrite | iGe::RHIDependencyAccess::DepthStencilAttachmentWrite;

    std::vector<iGe::RHISubpassDependency> dependencies = {dependency};

    iGe::RHIRenderPassCreateInfo passInfo{};
    passInfo.Attachments = attachments;
    passInfo.Subpasses = subpasses;
    passInfo.Dependencies = dependencies;

    m_RenderPass = iGe::RHI::Get()->CreateRenderPass(passInfo);
}

void ExampleLayer::CreateGraphicsPipeline() {
    // Define shader loader callback
    iGe::ShaderLoader shaderLoader = [](iGe::RHIShaderStage stage, const std::filesystem::path& path,
                                        const std::string& entryPoint) -> iGe::Scope<iGe::RHIShader> {
        std::ifstream file(path, std::ios::ate | std::ios::binary);
        if (!file.is_open()) { return nullptr; }

        size_t fileSize = static_cast<size_t>(file.tellg());
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        std::string source(buffer.begin(), buffer.end());

        iGe::RHIShaderCreateInfo info{};
        info.Stage = stage;
        info.SourceCode = source;
        info.EntryPoint = entryPoint;
        return iGe::RHI::Get()->CreateShader(info);
    };

    // Load Color pipeline (Triangle)
    m_TriGraphicsPipeline = iGe::PipelineParser::CreateGraphicsPipeline("assets/pipelines/Color.json", shaderLoader,
                                                                        m_RenderPass.get(), m_TriPipelineLayout.get());

    // Load Texture pipeline (Quad)
    m_QuadGraphicsPipeline = iGe::PipelineParser::CreateGraphicsPipeline("assets/pipelines/Texture.json", shaderLoader,
                                                                         m_RenderPass.get(), m_PipelineLayout.get());
}

void ExampleLayer::CreateBuffers() {
    auto rhi = iGe::RHI::Get();
    auto queue = rhi->GetQueue(iGe::RHIQueueType::Graphics);

    // Create checkerboard Texture
    {
        iGe::RHITextureCreateInfo texInfo{};
        texInfo.Extent = {256, 256, 1};
        texInfo.Format = iGe::RHIFormat::R8G8B8A8UNorm;
        texInfo.MipLevels = 1;
        texInfo.ArrayLayers = 1;
        texInfo.MemoryUsage = iGe::RHIMemoryUsage::GpuOnly;
        m_Texture = rhi->CreateTexture(texInfo);

        // Generate checkerboard pattern
        std::vector<uint32> textureData(256 * 256);
        for (int y = 0; y < 256; y++) {
            for (int x = 0; x < 256; x++) {
                if ((x / 16 + y / 16) % 2 == 0) {
                    textureData[y * 256 + x] = 0xFFFFFFFF; // White
                } else {
                    textureData[y * 256 + x] = 0xFF0000FF; // Red (ABGR format)
                }
            }
        }

        // Create staging buffer
        iGe::RHIBufferCreateInfo stagingInfo{};
        stagingInfo.Size = textureData.size() * sizeof(uint32);
        stagingInfo.Usage = iGe::RHIBufferUsageBit::TransferSrc;
        stagingInfo.MemoryUsage = iGe::RHIMemoryUsage::CpuToGpu;
        auto stagingBuffer = rhi->CreateBuffer(stagingInfo);

        if (void* ptr = stagingBuffer->Map()) {
            memcpy(ptr, textureData.data(), textureData.size() * sizeof(uint32));
            stagingBuffer->Unmap();
        }

        // Upload texture via command list
        auto cmdList = iGe::RHI::Get()->AllocateCommandList(m_CommandPool.Get());
        cmdList->Reset();
        cmdList->Begin();
        {
            cmdList->ResourceBarrier(m_Texture.get(), iGe::RHILayout::Undefined, iGe::RHILayout::TransferDst);
            cmdList->CopyBufferToTexture(stagingBuffer.get(), m_Texture.get());
            cmdList->ResourceBarrier(m_Texture.get(), iGe::RHILayout::TransferDst, iGe::RHILayout::ShaderReadOnly);
        }
        cmdList->End();

        queue->Submit(cmdList.get());
        queue->WaitIdle();

        // Create texture view for shader access
        iGe::RHITextureViewCreateInfo texViewInfo{};
        texViewInfo.ViewType = iGe::RHITextureViewType::View2D;
        texViewInfo.Format = iGe::RHIFormat::R8G8B8A8UNorm;
        m_TextureView = rhi->CreateTextureView(m_Texture.get(), texViewInfo);
    }

    // Create Triangle Vertex Buffer
    float triVertices[] = {
            // Position (x, y, z), Color (r, g, b)
            -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, // Red
            0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, // Green
            0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f  // Blue
    };
    iGe::RHIVertexBufferCreateInfo triVbInfo{};
    triVbInfo.Size = sizeof(triVertices);
    triVbInfo.Stride = 6 * sizeof(float);
    triVbInfo.MemoryUsage = iGe::RHIMemoryUsage::CpuToGpu;
    m_TriVertexBuffer = rhi->CreateVertexBuffer(triVbInfo);

    if (void* ptr = m_TriVertexBuffer->Map()) {
        memcpy(ptr, triVertices, sizeof(triVertices));
        m_TriVertexBuffer->Unmap();
    }

    // Create Triangle Index Buffer
    uint32 triIndices[] = {0, 1, 2};
    iGe::RHIIndexBufferCreateInfo triIbInfo{};
    triIbInfo.Size = sizeof(triIndices);
    triIbInfo.Format = iGe::RHIIndexFormat::Uint32;
    triIbInfo.MemoryUsage = iGe::RHIMemoryUsage::CpuToGpu;
    m_TriIndexBuffer = rhi->CreateIndexBuffer(triIbInfo);

    if (void* ptr = m_TriIndexBuffer->Map()) {
        memcpy(ptr, triIndices, sizeof(triIndices));
        m_TriIndexBuffer->Unmap();
    }

    // Create Quad Vertex Buffer
    float quadVertices[] = {
            // Position (x, y, z), TexCoord (u, v)
            -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, // Bottom-left
            0.5f,  -0.5f, 0.0f, 1.0f, 1.0f, // Bottom-right
            0.5f,  0.5f,  0.0f, 1.0f, 0.0f, // Top-right
            -0.5f, 0.5f,  0.0f, 0.0f, 0.0f  // Top-left
    };
    iGe::RHIVertexBufferCreateInfo quadVbInfo{};
    quadVbInfo.Size = sizeof(quadVertices);
    quadVbInfo.Stride = 5 * sizeof(float);
    quadVbInfo.MemoryUsage = iGe::RHIMemoryUsage::CpuToGpu;
    m_QuadVertexBuffer = rhi->CreateVertexBuffer(quadVbInfo);

    if (void* ptr = m_QuadVertexBuffer->Map()) {
        memcpy(ptr, quadVertices, sizeof(quadVertices));
        m_QuadVertexBuffer->Unmap();
    }

    // Create Quad Index Buffer
    uint32 quadIndices[] = {0, 1, 2, 2, 3, 0};
    iGe::RHIIndexBufferCreateInfo quadIbInfo{};
    quadIbInfo.Size = sizeof(quadIndices);
    quadIbInfo.Format = iGe::RHIIndexFormat::Uint32;
    quadIbInfo.MemoryUsage = iGe::RHIMemoryUsage::CpuToGpu;
    m_QuadIndexBuffer = rhi->CreateIndexBuffer(quadIbInfo);

    if (void* ptr = m_QuadIndexBuffer->Map()) {
        memcpy(ptr, quadIndices, sizeof(quadIndices));
        m_QuadIndexBuffer->Unmap();
    }

    // Create Uniform Buffer
    iGe::UniformBufferLayout layout = {{iGe::UBElementType::Float4x4, "ViewProjection"},
                                       {iGe::UBElementType::Float4x4, "Transform"}};
    iGe::RHIUniformBufferCreateInfo ubInfo{};
    ubInfo.Layout = layout;
    ubInfo.MemoryUsage = iGe::RHIMemoryUsage::CpuToGpu;
    m_UniformBuffer = rhi->CreateUniformBuffer(ubInfo);
}

void ExampleLayer::CreatePipelineLayout() {
    auto rhi = iGe::RHI::Get();

    // =========================================================================
    // Triangle pipeline layout (UBO only)
    // =========================================================================
    {
        // Binding 0: Uniform Buffer only
        std::vector<iGe::RHIDescriptorSetLayoutBinding> triBindings;
        iGe::RHIDescriptorSetLayoutBinding ubBinding{};
        ubBinding.Binding = 0;
        ubBinding.DescriptorType = iGe::RHIDescriptorType::UniformBuffer;
        ubBinding.DescriptorCount = 1;
        ubBinding.StageFlags = iGe::RHIShaderStage::Vertex;
        triBindings.push_back(ubBinding);

        iGe::RHIDescriptorSetLayoutCreateInfo triLayoutInfo{};
        triLayoutInfo.Bindings = triBindings;
        m_TriDescriptorSetLayout = rhi->CreateDescriptorSetLayout(triLayoutInfo);

        // Create triangle pipeline layout
        std::vector<const iGe::RHIDescriptorSetLayout*> triSetLayouts = {m_TriDescriptorSetLayout.get()};
        iGe::RHIPipelineLayoutCreateInfo triPipelineLayoutInfo{};
        triPipelineLayoutInfo.SetLayouts = triSetLayouts;
        m_TriPipelineLayout = rhi->CreatePipelineLayout(triPipelineLayoutInfo);
    }

    // =========================================================================
    // Quad pipeline layout (UBO + Texture + Sampler)
    // =========================================================================
    {
        std::vector<iGe::RHIDescriptorSetLayoutBinding> bindings;

        // Binding 0: Uniform Buffer
        iGe::RHIDescriptorSetLayoutBinding ubBinding{};
        ubBinding.Binding = 0;
        ubBinding.DescriptorType = iGe::RHIDescriptorType::UniformBuffer;
        ubBinding.DescriptorCount = 1;
        ubBinding.StageFlags = iGe::RHIShaderStage::Vertex | iGe::RHIShaderStage::Fragment;
        bindings.push_back(ubBinding);

        // Binding 1: Sampled Image (Texture)
        iGe::RHIDescriptorSetLayoutBinding texBinding{};
        texBinding.Binding = 1;
        texBinding.DescriptorType = iGe::RHIDescriptorType::SampledImage;
        texBinding.DescriptorCount = 1;
        texBinding.StageFlags = iGe::RHIShaderStage::Fragment;
        bindings.push_back(texBinding);

        // Binding 2: Sampler
        iGe::RHIDescriptorSetLayoutBinding samplerBinding{};
        samplerBinding.Binding = 2;
        samplerBinding.DescriptorType = iGe::RHIDescriptorType::Sampler;
        samplerBinding.DescriptorCount = 1;
        samplerBinding.StageFlags = iGe::RHIShaderStage::Fragment;
        bindings.push_back(samplerBinding);

        iGe::RHIDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.Bindings = bindings;
        m_DescriptorSetLayout = rhi->CreateDescriptorSetLayout(layoutInfo);

        // Create quad pipeline layout
        std::vector<const iGe::RHIDescriptorSetLayout*> setLayouts = {m_DescriptorSetLayout.get()};
        iGe::RHIPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.SetLayouts = setLayouts;
        m_PipelineLayout = rhi->CreatePipelineLayout(pipelineLayoutInfo);
    }
}

void ExampleLayer::CreateDescriptorResources() {
    auto rhi = iGe::RHI::Get();

    // Create sampler
    iGe::RHISamplerCreateInfo samplerInfo{};
    samplerInfo.MinFilter = iGe::RHISamplerFilter::Linear;
    samplerInfo.MagFilter = iGe::RHISamplerFilter::Linear;
    samplerInfo.MipmapMode = iGe::RHISamplerMipmapMode::Linear;
    samplerInfo.AddressModeU = iGe::RHISamplerAddressMode::Repeat;
    samplerInfo.AddressModeV = iGe::RHISamplerAddressMode::Repeat;
    samplerInfo.AddressModeW = iGe::RHISamplerAddressMode::Repeat;
    samplerInfo.MaxAnisotropy = 1.0f;
    samplerInfo.MinLod = 0.0f;
    samplerInfo.MaxLod = 1.0f;
    m_Sampler = rhi->CreateSampler(samplerInfo);

    // Create descriptor pool
    std::vector<iGe::RHIDescriptorPoolSize> poolSizes = {{iGe::RHIDescriptorType::UniformBuffer, 10},
                                                         {iGe::RHIDescriptorType::SampledImage, 10},
                                                         {iGe::RHIDescriptorType::Sampler, 10}};
    iGe::RHIDescriptorPoolCreateInfo poolInfo{};
    poolInfo.MaxSets = 10;
    poolInfo.PoolSizes = poolSizes;
    m_DescriptorPool = rhi->CreateDescriptorPool(poolInfo);

    // =========================================================================
    // Allocate and update Triangle descriptor set (UBO only)
    // =========================================================================
    {
        m_TriDescriptorSet = m_DescriptorPool->AllocateDescriptorSet(m_TriDescriptorSetLayout.get());

        // Update triangle descriptor set with UBO
        iGe::RHIDescriptorBufferInfo bufferInfo{};
        bufferInfo.pBuffer = m_UniformBuffer.get();
        bufferInfo.Offset = 0;
        bufferInfo.Range = ~0ULL;

        iGe::RHIWriteDescriptorSet triUbWrite{};
        triUbWrite.pDstSet = m_TriDescriptorSet.get();
        triUbWrite.DstBinding = 0;
        triUbWrite.DescriptorType = iGe::RHIDescriptorType::UniformBuffer;
        triUbWrite.DescriptorCount = 1;
        triUbWrite.pBufferInfos = &bufferInfo;

        std::array<iGe::RHIWriteDescriptorSet, 1> triWrites = {triUbWrite};
        rhi->UpdateDescriptorSets(triWrites);
    }

    // =========================================================================
    // Allocate and update Quad descriptor set (UBO + Texture + Sampler)
    // =========================================================================
    {
        m_DescriptorSet = m_DescriptorPool->AllocateDescriptorSet(m_DescriptorSetLayout.get());

        // Prepare buffer info
        iGe::RHIDescriptorBufferInfo bufferInfo{};
        bufferInfo.pBuffer = m_UniformBuffer.get();
        bufferInfo.Offset = 0;
        bufferInfo.Range = ~0ULL;

        // Prepare image infos
        iGe::RHIDescriptorImageInfo texImageInfo{};
        texImageInfo.pSampler = nullptr;
        texImageInfo.pTextureView = m_TextureView.get();
        texImageInfo.ImageLayout = iGe::RHILayout::ShaderReadOnly;

        iGe::RHIDescriptorImageInfo samplerImageInfo{};
        samplerImageInfo.pSampler = m_Sampler.get();
        samplerImageInfo.pTextureView = nullptr;
        samplerImageInfo.ImageLayout = iGe::RHILayout::Undefined;

        // Write Uniform Buffer at binding 0
        iGe::RHIWriteDescriptorSet ubWrite{};
        ubWrite.pDstSet = m_DescriptorSet.get();
        ubWrite.DstBinding = 0;
        ubWrite.DescriptorType = iGe::RHIDescriptorType::UniformBuffer;
        ubWrite.DescriptorCount = 1;
        ubWrite.pBufferInfos = &bufferInfo;

        // Write Texture at binding 1
        iGe::RHIWriteDescriptorSet texWrite{};
        texWrite.pDstSet = m_DescriptorSet.get();
        texWrite.DstBinding = 1;
        texWrite.DescriptorType = iGe::RHIDescriptorType::SampledImage;
        texWrite.DescriptorCount = 1;
        texWrite.pImageInfos = &texImageInfo;

        // Write Sampler at binding 2
        iGe::RHIWriteDescriptorSet samplerWrite{};
        samplerWrite.pDstSet = m_DescriptorSet.get();
        samplerWrite.DstBinding = 2;
        samplerWrite.DescriptorType = iGe::RHIDescriptorType::Sampler;
        samplerWrite.DescriptorCount = 1;
        samplerWrite.pImageInfos = &samplerImageInfo;

        // Batch update
        std::array<iGe::RHIWriteDescriptorSet, 3> writes = {ubWrite, texWrite, samplerWrite};
        rhi->UpdateDescriptorSets(writes);
    }
}

void ExampleLayer::CreateDepthResources(uint32 width, uint32 height) {
    auto rhi = iGe::RHI::Get();
    auto queue = rhi->GetQueue(iGe::RHIQueueType::Graphics);

    // Create depth texture
    iGe::RHITextureCreateInfo depthInfo{};
    depthInfo.Extent = {width, height, 1};
    depthInfo.Format = iGe::RHIFormat::D32SFloat;
    depthInfo.MipLevels = 1;
    depthInfo.ArrayLayers = 1;
    depthInfo.MemoryUsage = iGe::RHIMemoryUsage::GpuOnly;
    m_DepthAttachment = rhi->CreateTexture(depthInfo);

    // Create depth texture view
    iGe::RHITextureViewCreateInfo depthViewInfo{};
    depthViewInfo.ViewType = iGe::RHITextureViewType::View2D;
    depthViewInfo.Format = iGe::RHIFormat::D32SFloat;
    m_DepthTextureView = rhi->CreateTextureView(m_DepthAttachment.get(), depthViewInfo);

    // Transition depth buffer to DepthStencilAttachment state immediately after creation
    // This way we don't need to transition it every fram
    auto cmdList = iGe::RHI::Get()->AllocateCommandList(m_CommandPool.Get());
    cmdList->Reset();
    cmdList->Begin();
    cmdList->ResourceBarrier(m_DepthAttachment.get(), iGe::RHILayout::Undefined,
                             iGe::RHILayout::DepthStencilAttachment);
    cmdList->End();
    queue->Submit(cmdList.get());
    queue->WaitIdle();
}
