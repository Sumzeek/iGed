module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <dxgi1_6.h>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12RHI;
import :DirectX12Surface;
import :DirectX12Queue;
import :DirectX12CommandPool;
import :DirectX12CommandList;
import :DirectX12Buffer;
import :DirectX12Texture;
import :DirectX12TextureView;
import :DirectX12SwapChain;
import :DirectX12Shader;
import :DirectX12GraphicsPipeline;
import :DirectX12ComputePipeline;
import :DirectX12RenderPass;
import :DirectX12Framebuffer;
import :DirectX12Fence;
import :DirectX12Semaphore;
import :DirectX12Sampler;
import :DirectX12Descriptor;
import :DirectX12Helper;

namespace iGe
{

// =================================================================================================
// DirectX12RHI
// =================================================================================================

DirectX12RHI::DirectX12RHI() { Init(); }

DirectX12RHI::~DirectX12RHI() {
    WaitIdle();

    // Clean up queues
    m_GraphicsQueue.reset();
    m_ComputeQueue.reset();
    m_TransferQueue.reset();

    if (m_CallbackCookie != 0 && m_Device) {
        Microsoft::WRL::ComPtr<ID3D12InfoQueue1> infoQueue1;
        if (SUCCEEDED(m_Device.As(&infoQueue1))) { infoQueue1->UnregisterMessageCallback(m_CallbackCookie); }
        m_CallbackCookie = 0;
    }
}

void DirectX12RHI::Init() {
    // Enable Debug Layer
    UINT dxgiFactoryFlags = 0;
    #if defined(IGE_DEBUG)
    {
        Microsoft::WRL::ComPtr<ID3D12Debug> debug;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)))) {
            debug->EnableDebugLayer();

            Microsoft::WRL::ComPtr<ID3D12Debug1> debug1;
            if (SUCCEEDED(debug.As(&debug1))) { debug1->SetEnableSynchronizedCommandQueueValidation(TRUE); }
        }
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }
    #endif

    HRESULT hr = CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&m_Factory));
    if (FAILED(hr)) { Internal::LogError("Failed to create DXGI Factory"); }

    // Use the factory to find an adapter
    for (UINT i = 0; m_Factory->EnumAdapters1(i, &m_Adapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        DXGI_ADAPTER_DESC1 desc;
        m_Adapter->GetDesc1(&desc);
        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;

        if (SUCCEEDED(D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), nullptr))) {
            break;
        }
        m_Adapter = nullptr;
    }

    if (!m_Adapter) { m_Factory->EnumWarpAdapter(IID_PPV_ARGS(&m_Adapter)); }

    if (!m_Adapter) { Internal::LogError("Failed to find a compatible adapter"); }

    // Create Device
    hr = D3D12CreateDevice(m_Adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device));
    if (FAILED(hr)) { Internal::LogError("Failed to create D3D12 Device"); }

    // Try to get Device5 for ray tracing
    if (SUCCEEDED(m_Device.As(&m_Device5))) {
        D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};
        if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &options5, sizeof(options5)))) {
            m_RayTracingSupported = (options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_0);
        }
    }

    // Create Queues
    auto createQueue = [&](D3D12_COMMAND_LIST_TYPE type, RHIQueueType rhiType) -> Scope<DirectX12Queue> {
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        queueDesc.Type = type;

        Microsoft::WRL::ComPtr<ID3D12CommandQueue> queue;
        if (FAILED(m_Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)))) {
            Internal::LogError("Failed to create command queue.");
        }

        RHIQueueCreateInfo info = {};
        info.Type = rhiType;
        return CreateScope<DirectX12Queue>(info, queue);
    };

    m_GraphicsQueue = createQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, RHIQueueType::Graphics);
    m_ComputeQueue = createQueue(D3D12_COMMAND_LIST_TYPE_COMPUTE, RHIQueueType::Compute);
    m_TransferQueue = createQueue(D3D12_COMMAND_LIST_TYPE_COPY, RHIQueueType::Transfer);

    // Initialize staging heaps and device properties
    InitStagingHeaps();
    InitDeviceProperties();

    #if defined(IGE_DEBUG)
    Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(m_Device.As(&infoQueue))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
    }

    Microsoft::WRL::ComPtr<ID3D12InfoQueue1> infoQueue1;
    if (SUCCEEDED(m_Device.As(&infoQueue1))) {
        auto callback = [](D3D12_MESSAGE_CATEGORY Category, D3D12_MESSAGE_SEVERITY Severity, D3D12_MESSAGE_ID ID,
                           LPCSTR pDescription, void* pContext) {
            if (Severity == D3D12_MESSAGE_SEVERITY_ERROR || Severity == D3D12_MESSAGE_SEVERITY_CORRUPTION) {
                Internal::LogError("[D3D12 Error] {}", pDescription);
            } else if (Severity == D3D12_MESSAGE_SEVERITY_WARNING) {
                Internal::LogWarn("[D3D12 Warning] {}", pDescription);
            } else {
                Internal::LogInfo("[D3D12 Info] {}", pDescription);
            }
        };

        infoQueue1->RegisterMessageCallback(callback, D3D12_MESSAGE_CALLBACK_FLAG_NONE, nullptr, &m_CallbackCookie);
    }
    #endif
}

void DirectX12RHI::InitDeviceProperties() {
    DXGI_ADAPTER_DESC1 adapterDesc;
    m_Adapter->GetDesc1(&adapterDesc);

    // Convert wide string to narrow string
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    m_DeviceProperties.DeviceName = converter.to_bytes(adapterDesc.Description);
    m_DeviceProperties.VendorID = adapterDesc.VendorId;
    m_DeviceProperties.DeviceID = adapterDesc.DeviceId;

    // Set limits
    m_DeviceProperties.Limits.MaxImageDimension1D = 16384;
    m_DeviceProperties.Limits.MaxImageDimension2D = 16384;
    m_DeviceProperties.Limits.MaxImageDimension3D = 2048;
    m_DeviceProperties.Limits.MaxImageDimensionCube = 16384;
    m_DeviceProperties.Limits.MaxImageArrayLayers = 2048;
    m_DeviceProperties.Limits.MaxUniformBufferRange = 65536;
    m_DeviceProperties.Limits.MaxStorageBufferRange = 2147483647;
    m_DeviceProperties.Limits.MaxPushConstantsSize = 256; // D3D12 root constants limit
    m_DeviceProperties.Limits.MaxBoundDescriptorSets = 8;
    m_DeviceProperties.Limits.MaxColorAttachments = D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT;
    m_DeviceProperties.Limits.MaxComputeWorkGroupInvocations = 1024;
    m_DeviceProperties.Limits.MaxComputeWorkGroupSize[0] = 1024;
    m_DeviceProperties.Limits.MaxComputeWorkGroupSize[1] = 1024;
    m_DeviceProperties.Limits.MaxComputeWorkGroupSize[2] = 64;

    // Memory properties (simplified for D3D12)
    m_MemoryProperties.Heaps.resize(3);
    // Heap 0: GPU memory (VRAM)
    m_MemoryProperties.Heaps[0].Size = adapterDesc.DedicatedVideoMemory;
    m_MemoryProperties.Heaps[0].DeviceLocal = true;
    // Heap 1: System memory for upload
    m_MemoryProperties.Heaps[1].Size = adapterDesc.SharedSystemMemory;
    m_MemoryProperties.Heaps[1].DeviceLocal = false;
    // Heap 2: Readback
    m_MemoryProperties.Heaps[2].Size = adapterDesc.SharedSystemMemory;
    m_MemoryProperties.Heaps[2].DeviceLocal = false;
}

void DirectX12RHI::InitStagingHeaps() {
    m_CBVSRVUAVStagingHeap.Initialize(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4096);
    m_SamplerStagingHeap.Initialize(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, 256);
    m_RTVStagingHeap.Initialize(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 256);
    m_DSVStagingHeap.Initialize(m_Device.Get(), D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 64);
}

void DirectX12RHI::WaitIdle() {
    if (m_GraphicsQueue) { m_GraphicsQueue->WaitIdle(); }
    if (m_ComputeQueue) { m_ComputeQueue->WaitIdle(); }
    if (m_TransferQueue) { m_TransferQueue->WaitIdle(); }
}

RHIFormatProperties DirectX12RHI::GetFormatProperties(RHIFormat format) const {
    RHIFormatProperties props = {};
    DXGI_FORMAT dxFormat = RHIFormatToDXGIFormat(format);

    D3D12_FEATURE_DATA_FORMAT_SUPPORT formatSupport = {};
    formatSupport.Format = dxFormat;

    if (SUCCEEDED(m_Device->CheckFeatureSupport(D3D12_FEATURE_FORMAT_SUPPORT, &formatSupport, sizeof(formatSupport)))) {
        // Optimal tiling features
        props.OptimalTilingSampledImage = (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D) != 0;
        props.OptimalTilingColorAttachment = (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0;
        props.OptimalTilingDepthStencilAttachment = (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_DEPTH_STENCIL) != 0;
        props.OptimalTilingStorageImage = (formatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_STORE) != 0;
        props.OptimalTilingBlitSrc = (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_TEXTURE2D) != 0;
        props.OptimalTilingBlitDst = (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_RENDER_TARGET) != 0;

        // Buffer features
        props.BufferVertexBuffer = (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_IA_VERTEX_BUFFER) != 0;
        props.BufferUniformTexelBuffer = (formatSupport.Support1 & D3D12_FORMAT_SUPPORT1_BUFFER) != 0;
        props.BufferStorageTexelBuffer = (formatSupport.Support2 & D3D12_FORMAT_SUPPORT2_UAV_TYPED_LOAD) != 0;
    }

    return props;
}

// =============================================================================
// Queue Operations
// =============================================================================

RHIQueue* DirectX12RHI::GetQueue(RHIQueueType type, uint32 index) {
    switch (type) {
        case RHIQueueType::Graphics:
            return m_GraphicsQueue.Get();
        case RHIQueueType::Compute:
            return m_ComputeQueue.Get();
        case RHIQueueType::Transfer:
            return m_TransferQueue.Get();
        default:
            return nullptr;
    }
}

uint32 DirectX12RHI::GetQueueCount(RHIQueueType type) const {
    // DirectX12 creates one queue per type in our implementation
    switch (type) {
        case RHIQueueType::Graphics:
            return m_GraphicsQueue ? 1 : 0;
        case RHIQueueType::Compute:
            return m_ComputeQueue ? 1 : 0;
        case RHIQueueType::Transfer:
            return m_TransferQueue ? 1 : 0;
        default:
            return 0;
    }
}

// =============================================================================
// Surface and SwapChain
// =============================================================================

Scope<RHISurface> DirectX12RHI::CreateSurface(const RHISurfaceCreateInfo& info) {
    return CreateScope<DirectX12Surface>(info);
}

Scope<RHISwapChain> DirectX12RHI::CreateSwapChain(const RHISwapChainCreateInfo& info) {
    return CreateScope<DirectX12SwapChain>(m_Factory.Get(), m_Device.Get(), m_GraphicsQueue->GetCommandQueue(), info);
}

// =============================================================================
// Command Infrastructure
// =============================================================================

Scope<RHICommandPool> DirectX12RHI::CreateCommandPool(const RHICommandPoolCreateInfo& info) {
    return CreateScope<DirectX12CommandPool>(m_Device.Get(), info);
}

Scope<RHICommandList> DirectX12RHI::AllocateCommandList(RHICommandPool* pPool) {
    if (!pPool) { return nullptr; }
    return CreateScope<DirectX12CommandList>(m_Device.Get(), pPool);
}

std::vector<Scope<RHICommandList>> DirectX12RHI::AllocateCommandLists(RHICommandPool* pPool, uint32 count) {
    if (!pPool) { return {}; }

    std::vector<Scope<RHICommandList>> commandLists;

    commandLists.reserve(count);
    for (uint32 i = 0; i < count; ++i) { commandLists.push_back(this->AllocateCommandList(pPool)); }
    return commandLists;
}

void DirectX12RHI::FreeCommandList(RHICommandPool* pPool, RHICommandList* pCommandList) {
    // DX12: Command List is automatically managed through smart Pointers without the need for explicit release
}

void DirectX12RHI::FreeCommandLists(RHICommandPool* pPool, std::span<RHICommandList*> commandLists) {
    // DX12: Command List is automatically managed through smart Pointers without the need for explicit release
}

// =============================================================================
// Buffer Operations
// =============================================================================

Scope<RHIBuffer> DirectX12RHI::CreateBuffer(const RHIBufferCreateInfo& info) {
    return CreateScope<DirectX12Buffer>(m_Device.Get(), info);
}

Scope<RHIVertexBuffer> DirectX12RHI::CreateVertexBuffer(const RHIVertexBufferCreateInfo& info) {
    return CreateScope<DirectX12VertexBuffer>(m_Device.Get(), info);
}

Scope<RHIIndexBuffer> DirectX12RHI::CreateIndexBuffer(const RHIIndexBufferCreateInfo& info) {
    return CreateScope<DirectX12IndexBuffer>(m_Device.Get(), info);
}

Scope<RHIUniformBuffer> DirectX12RHI::CreateUniformBuffer(const RHIUniformBufferCreateInfo& info) {
    return CreateScope<DirectX12UniformBuffer>(m_Device.Get(), info);
}

Scope<RHIStorageBuffer> DirectX12RHI::CreateStorageBuffer(const RHIStorageBufferCreateInfo& info) {
    return CreateScope<DirectX12StorageBuffer>(m_Device.Get(), info);
}

// =============================================================================
// Texture Operations
// =============================================================================

Scope<RHITexture> DirectX12RHI::CreateTexture(const RHITextureCreateInfo& info) {
    return CreateScope<DirectX12Texture>(m_Device.Get(), info);
}

Scope<RHITextureView> DirectX12RHI::CreateTextureView(const RHITexture* pTexture,
                                                      const RHITextureViewCreateInfo& info) {
    if (!pTexture) {
        Internal::LogError("CreateTextureView: Texture is null");
        return nullptr;
    }

    auto dx12Texture = dynamic_cast<const DirectX12Texture*>(pTexture);
    if (!dx12Texture) {
        Internal::LogError("CreateTextureView: Texture is not a DirectX12Texture");
        return nullptr;
    }

    return CreateScope<DirectX12TextureView>(m_Device.Get(), info, const_cast<DirectX12Texture*>(dx12Texture));
}

// =============================================================================
// Sampler Operations
// =============================================================================

Scope<RHISampler> DirectX12RHI::CreateSampler(const RHISamplerCreateInfo& info) {
    return CreateScope<DirectX12Sampler>(m_Device.Get(), info);
}

// =============================================================================
// Descriptor Operations
// =============================================================================

Scope<RHIDescriptorSetLayout> DirectX12RHI::CreateDescriptorSetLayout(const RHIDescriptorSetLayoutCreateInfo& info) {
    return CreateScope<DirectX12DescriptorSetLayout>(info);
}

Scope<RHIDescriptorPool> DirectX12RHI::CreateDescriptorPool(const RHIDescriptorPoolCreateInfo& info) {
    return CreateScope<DirectX12DescriptorPool>(m_Device.Get(), info);
}

void DirectX12RHI::UpdateDescriptorSets(std::span<const RHIWriteDescriptorSet> writes) {
    if (writes.empty()) { return; }

    for (const auto& write: writes) {
        if (!write.pDstSet) continue;

        auto* dx12Set = static_cast<DirectX12DescriptorSet*>(write.pDstSet);
        auto* pool = dx12Set->GetPool();
        auto* layout = dx12Set->GetLayout();
        if (!pool || !layout) continue;

        const auto& bindings = layout->GetBindings();

        // Find the binding offset
        uint32 cbvSrvUavOffset = 0;
        uint32 samplerOffset = 0;

        for (const auto& binding: bindings) {
            if (binding.Binding == write.DstBinding) { break; }
            if (binding.DescriptorType == RHIDescriptorType::Sampler) {
                samplerOffset += binding.DescriptorCount;
            } else {
                cbvSrvUavOffset += binding.DescriptorCount;
            }
        }

        // Update descriptors based on type
        if (write.DescriptorType == RHIDescriptorType::Sampler) {
            if (!write.pImageInfos) continue;
            uint32 dstIndex = dx12Set->GetSamplerStartIndex() + samplerOffset + write.DstArrayElement;
            for (uint32 i = 0; i < write.DescriptorCount; ++i) {
                auto* dx12Sampler = static_cast<const DirectX12Sampler*>(write.pImageInfos[i].pSampler);
                if (dx12Sampler) {
                    D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = pool->GetSamplerCPUHandle(dstIndex + i);
                    m_Device->CreateSampler(&dx12Sampler->GetDesc(), dstHandle);
                }
            }
        } else if (write.DescriptorType == RHIDescriptorType::UniformBuffer ||
                   write.DescriptorType == RHIDescriptorType::UniformBufferDynamic) {
            if (!write.pBufferInfos) continue;
            uint32 dstIndex = dx12Set->GetCBVSRVUAVStartIndex() + cbvSrvUavOffset + write.DstArrayElement;
            for (uint32 i = 0; i < write.DescriptorCount; ++i) {
                auto* buffer = static_cast<const DirectX12Buffer*>(write.pBufferInfos[i].pBuffer);
                if (buffer) {
                    D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
                    cbvDesc.BufferLocation =
                            buffer->GetResource()->GetGPUVirtualAddress() + write.pBufferInfos[i].Offset;
                    uint64 range = write.pBufferInfos[i].Range;
                    if (range == ~0ULL) { range = buffer->GetSize() - write.pBufferInfos[i].Offset; }
                    cbvDesc.SizeInBytes = static_cast<UINT>((range + 255) & ~255); // 256-byte aligned

                    D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = pool->GetCBVSRVUAVCPUHandle(dstIndex + i);
                    m_Device->CreateConstantBufferView(&cbvDesc, dstHandle);
                }
            }
        } else if (write.DescriptorType == RHIDescriptorType::StorageBuffer ||
                   write.DescriptorType == RHIDescriptorType::StorageBufferDynamic) {
            if (!write.pBufferInfos) continue;
            uint32 dstIndex = dx12Set->GetCBVSRVUAVStartIndex() + cbvSrvUavOffset + write.DstArrayElement;
            for (uint32 i = 0; i < write.DescriptorCount; ++i) {
                auto* buffer = static_cast<const DirectX12Buffer*>(write.pBufferInfos[i].pBuffer);
                if (buffer) {
                    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
                    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
                    uavDesc.Buffer.FirstElement = write.pBufferInfos[i].Offset / 4;
                    uint64 range = write.pBufferInfos[i].Range;
                    if (range == ~0ULL) { range = buffer->GetSize() - write.pBufferInfos[i].Offset; }
                    uavDesc.Buffer.NumElements = static_cast<UINT>(range / 4);
                    uavDesc.Buffer.StructureByteStride = 0;
                    uavDesc.Format = DXGI_FORMAT_R32_TYPELESS;
                    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

                    D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = pool->GetCBVSRVUAVCPUHandle(dstIndex + i);
                    m_Device->CreateUnorderedAccessView(buffer->GetResource(), nullptr, &uavDesc, dstHandle);
                }
            }
        } else if (write.DescriptorType == RHIDescriptorType::SampledImage ||
                   write.DescriptorType == RHIDescriptorType::CombinedImageSampler) {
            if (!write.pImageInfos) continue;
            uint32 dstIndex = dx12Set->GetCBVSRVUAVStartIndex() + cbvSrvUavOffset + write.DstArrayElement;
            for (uint32 i = 0; i < write.DescriptorCount; ++i) {
                auto* textureView = static_cast<const DirectX12TextureView*>(write.pImageInfos[i].pTextureView);
                if (textureView) {
                    D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = textureView->GetSRVCpu();
                    D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = pool->GetCBVSRVUAVCPUHandle(dstIndex + i);

                    if (srcHandle.ptr != 0) {
                        m_Device->CopyDescriptorsSimple(1, dstHandle, srcHandle,
                                                        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    }
                }
            }
        } else if (write.DescriptorType == RHIDescriptorType::StorageImage) {
            if (!write.pImageInfos) continue;
            uint32 dstIndex = dx12Set->GetCBVSRVUAVStartIndex() + cbvSrvUavOffset + write.DstArrayElement;
            for (uint32 i = 0; i < write.DescriptorCount; ++i) {
                auto* textureView = static_cast<const DirectX12TextureView*>(write.pImageInfos[i].pTextureView);
                if (textureView) {
                    D3D12_CPU_DESCRIPTOR_HANDLE srcHandle = textureView->GetUAVCpu();
                    D3D12_CPU_DESCRIPTOR_HANDLE dstHandle = pool->GetCBVSRVUAVCPUHandle(dstIndex + i);

                    if (srcHandle.ptr != 0) {
                        m_Device->CopyDescriptorsSimple(1, dstHandle, srcHandle,
                                                        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                    }
                }
            }
        }
    }
}

void DirectX12RHI::CopyDescriptorSets(std::span<const RHICopyDescriptorSet> copies) {
    if (copies.empty()) return;

    for (const auto& copy: copies) {
        auto* srcSet = static_cast<const DirectX12DescriptorSet*>(copy.pSrcSet);
        auto* dstSet = static_cast<DirectX12DescriptorSet*>(copy.pDstSet);
        if (!srcSet || !dstSet) continue;

        auto* srcPool = srcSet->GetPool();
        auto* dstPool = dstSet->GetPool();
        if (!srcPool || !dstPool) continue;

        // Find binding offsets for source and destination
        // This is simplified - assumes bindings are in order
        uint32 srcOffset = copy.SrcBinding + copy.SrcArrayElement;
        uint32 dstOffset = copy.DstBinding + copy.DstArrayElement;

        // Copy CBV/SRV/UAV descriptors
        if (srcSet->HasCBVSRVUAV() && dstSet->HasCBVSRVUAV()) {
            D3D12_CPU_DESCRIPTOR_HANDLE srcHandle =
                    srcPool->GetCBVSRVUAVCPUHandle(srcSet->GetCBVSRVUAVStartIndex() + srcOffset);
            D3D12_CPU_DESCRIPTOR_HANDLE dstHandle =
                    dstPool->GetCBVSRVUAVCPUHandle(dstSet->GetCBVSRVUAVStartIndex() + dstOffset);

            m_Device->CopyDescriptorsSimple(copy.DescriptorCount, dstHandle, srcHandle,
                                            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        }

        // Copy Sampler descriptors
        if (srcSet->HasSamplers() && dstSet->HasSamplers()) {
            D3D12_CPU_DESCRIPTOR_HANDLE srcHandle =
                    srcPool->GetSamplerCPUHandle(srcSet->GetSamplerStartIndex() + srcOffset);
            D3D12_CPU_DESCRIPTOR_HANDLE dstHandle =
                    dstPool->GetSamplerCPUHandle(dstSet->GetSamplerStartIndex() + dstOffset);

            m_Device->CopyDescriptorsSimple(copy.DescriptorCount, dstHandle, srcHandle,
                                            D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
        }
    }
}

// =============================================================================
// Pipeline Layout
// =============================================================================

Scope<RHIPipelineLayout> DirectX12RHI::CreatePipelineLayout(const RHIPipelineLayoutCreateInfo& info) {
    return CreateScope<DirectX12PipelineLayout>(m_Device.Get(), info);
}

// =============================================================================
// Render Pass and Framebuffer
// =============================================================================

Scope<RHIRenderPass> DirectX12RHI::CreateRenderPass(const RHIRenderPassCreateInfo& info) {
    return CreateScope<DirectX12RenderPass>(info);
}

Scope<RHIFramebuffer> DirectX12RHI::CreateFramebuffer(const RHIFramebufferCreateInfo& info) {
    return CreateScope<DirectX12Framebuffer>(m_Device.Get(), info);
}

// =============================================================================
// Shader Operations
// =============================================================================

Scope<RHIShader> DirectX12RHI::CreateShader(const RHIShaderCreateInfo& info) {
    return CreateScope<DirectX12Shader>(info);
}

// =============================================================================
// Pipeline Operations
// =============================================================================

Scope<RHIGraphicsPipeline> DirectX12RHI::CreateGraphicsPipeline(const RHIGraphicsPipelineCreateInfo& info) {
    return CreateScope<DirectX12GraphicsPipeline>(m_Device.Get(), info);
}

Scope<RHIComputePipeline> DirectX12RHI::CreateComputePipeline(const RHIComputePipelineCreateInfo& info) {
    return CreateScope<DirectX12ComputePipeline>(m_Device.Get(), info);
}

// =============================================================================
// Synchronization Primitives
// =============================================================================

Scope<RHIFence> DirectX12RHI::CreateGPUFence(const RHIFenceCreateInfo& info) {
    return CreateScope<DirectX12Fence>(m_Device.Get(), info);
}

Scope<RHISemaphore> DirectX12RHI::CreateGPUSemaphore() { return CreateScope<DirectX12Semaphore>(m_Device.Get()); }

bool DirectX12RHI::WaitForFences(std::span<RHIFence* const> fences, bool waitAll, uint64 timeout) {
    if (fences.empty()) { return true; }

    for (auto* fence: fences) {
        if (!fence) { continue; }

        auto dxFence = dynamic_cast<DirectX12Fence*>(fence);
        if (!dxFence) { continue; }

        bool result = dxFence->Wait(timeout);
        if (!result && waitAll) { return false; }
        if (result && !waitAll) { return true; }
    }

    return waitAll;
}

void DirectX12RHI::ResetFences(std::span<RHIFence* const> fences) {
    if (fences.empty()) { return; }

    for (auto* fence: fences) {
        if (!fence) { continue; }

        auto dxFence = dynamic_cast<DirectX12Fence*>(fence);
        if (dxFence) { dxFence->Reset(); }
    }
}

// =============================================================================
// Resource Destruction
// =============================================================================

void DirectX12RHI::DestroyResource(RHIResource* pResource) {
    if (!pResource) { return; }

    // The destructor of each resource type will handle cleanup
    delete pResource;
}

} // namespace iGe
#endif
