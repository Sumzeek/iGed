module;
#include "iGeMacro.h"

export module iGe.RHI:RHIDeviceCapabilities;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// Physical Device Type
// =================================================================================================

export enum class RHIPhysicalDeviceType : uint32 {
    Other = 0,
    IntegratedGpu,
    DiscreteGpu,
    VirtualGpu,
    Cpu,

    Count
};

// =================================================================================================
// Device Limits
// =================================================================================================

export struct RHIDeviceLimits {
    // Image/Texture limits
    uint32 MaxImageDimension1D = 0;
    uint32 MaxImageDimension2D = 0;
    uint32 MaxImageDimension3D = 0;
    uint32 MaxImageDimensionCube = 0;
    uint32 MaxImageArrayLayers = 0;

    // Buffer limits
    uint32 MaxUniformBufferRange = 0;
    uint32 MaxStorageBufferRange = 0;
    uint32 MaxPushConstantsSize = 0;

    // Memory limits
    uint32 MaxMemoryAllocationCount = 0;
    uint64 BufferImageGranularity = 0;
    uint64 SparseAddressSpaceSize = 0;
    uint64 NonCoherentAtomSize = 0;

    // Descriptor limits
    uint32 MaxBoundDescriptorSets = 0;
    uint32 MaxPerStageDescriptorSamplers = 0;
    uint32 MaxPerStageDescriptorUniformBuffers = 0;
    uint32 MaxPerStageDescriptorStorageBuffers = 0;
    uint32 MaxPerStageDescriptorSampledImages = 0;
    uint32 MaxPerStageDescriptorStorageImages = 0;
    uint32 MaxPerStageDescriptorInputAttachments = 0;
    uint32 MaxPerStageResources = 0;
    uint32 MaxDescriptorSetSamplers = 0;
    uint32 MaxDescriptorSetUniformBuffers = 0;
    uint32 MaxDescriptorSetUniformBuffersDynamic = 0;
    uint32 MaxDescriptorSetStorageBuffers = 0;
    uint32 MaxDescriptorSetStorageBuffersDynamic = 0;
    uint32 MaxDescriptorSetSampledImages = 0;
    uint32 MaxDescriptorSetStorageImages = 0;
    uint32 MaxDescriptorSetInputAttachments = 0;

    // Vertex input limits
    uint32 MaxVertexInputAttributes = 0;
    uint32 MaxVertexInputBindings = 0;
    uint32 MaxVertexInputAttributeOffset = 0;
    uint32 MaxVertexInputBindingStride = 0;
    uint32 MaxVertexOutputComponents = 0;

    // Tessellation limits
    uint32 MaxTessellationGenerationLevel = 0;
    uint32 MaxTessellationPatchSize = 0;
    uint32 MaxTessellationControlPerVertexInputComponents = 0;
    uint32 MaxTessellationControlPerVertexOutputComponents = 0;
    uint32 MaxTessellationControlPerPatchOutputComponents = 0;
    uint32 MaxTessellationControlTotalOutputComponents = 0;
    uint32 MaxTessellationEvaluationInputComponents = 0;
    uint32 MaxTessellationEvaluationOutputComponents = 0;

    // Geometry shader limits
    uint32 MaxGeometryShaderInvocations = 0;
    uint32 MaxGeometryInputComponents = 0;
    uint32 MaxGeometryOutputComponents = 0;
    uint32 MaxGeometryOutputVertices = 0;
    uint32 MaxGeometryTotalOutputComponents = 0;

    // Fragment shader limits
    uint32 MaxFragmentInputComponents = 0;
    uint32 MaxFragmentOutputAttachments = 0;
    uint32 MaxFragmentDualSrcAttachments = 0;
    uint32 MaxFragmentCombinedOutputResources = 0;

    // Compute shader limits
    uint32 MaxComputeSharedMemorySize = 0;
    uint32 MaxComputeWorkGroupCount[3] = {0, 0, 0};
    uint32 MaxComputeWorkGroupInvocations = 0;
    uint32 MaxComputeWorkGroupSize[3] = {0, 0, 0};

    // Viewport limits
    uint32 MaxViewports = 0;
    uint32 MaxViewportDimensions[2] = {0, 0};
    float ViewportBoundsRange[2] = {0.0f, 0.0f};

    // Framebuffer limits
    uint32 MaxFramebufferWidth = 0;
    uint32 MaxFramebufferHeight = 0;
    uint32 MaxFramebufferLayers = 0;
    uint32 MaxColorAttachments = 0;

    // Sample counts (flags)
    uint32 FramebufferColorSampleCounts = 0;
    uint32 FramebufferDepthSampleCounts = 0;
    uint32 FramebufferStencilSampleCounts = 0;
    uint32 FramebufferNoAttachmentsSampleCounts = 0;
    uint32 SampledImageColorSampleCounts = 0;
    uint32 SampledImageIntegerSampleCounts = 0;
    uint32 SampledImageDepthSampleCounts = 0;
    uint32 SampledImageStencilSampleCounts = 0;
    uint32 StorageImageSampleCounts = 0;

    // Other limits
    float MaxSamplerAnisotropy = 0.0f;
    float MaxSamplerLodBias = 0.0f;
    uint32 MaxDrawIndexedIndexValue = 0;
    uint32 MaxDrawIndirectCount = 0;
    float PointSizeRange[2] = {0.0f, 0.0f};
    float LineWidthRange[2] = {0.0f, 0.0f};
    float PointSizeGranularity = 0.0f;
    float LineWidthGranularity = 0.0f;

    // Alignment requirements
    uint64 MinTexelBufferOffsetAlignment = 0;
    uint64 MinUniformBufferOffsetAlignment = 0;
    uint64 MinStorageBufferOffsetAlignment = 0;
    uint64 OptimalBufferCopyOffsetAlignment = 0;
    uint64 OptimalBufferCopyRowPitchAlignment = 0;

    // Timestamp period (nanoseconds per timestamp increment)
    float TimestampPeriod = 0.0f;
    bool TimestampComputeAndGraphics = false;
};

// =================================================================================================
// Device Features
// =================================================================================================

export struct RHIDeviceFeatures {
    // Geometry and tessellation
    bool GeometryShader = false;
    bool TessellationShader = false;

    // Shader capabilities
    bool ShaderFloat64 = false;
    bool ShaderInt64 = false;
    bool ShaderInt16 = false;

    // Multi-draw
    bool MultiDrawIndirect = false;
    bool DrawIndirectFirstInstance = false;

    // Depth/Stencil
    bool DepthClamp = false;
    bool DepthBiasClamp = false;
    bool DepthBounds = false;

    // Fill modes
    bool FillModeNonSolid = false; // Wireframe
    bool WideLines = false;
    bool LargePoints = false;

    // Blending
    bool IndependentBlend = false;
    bool DualSrcBlend = false;
    bool LogicOp = false;

    // Multisampling
    bool SampleRateShading = false;
    bool AlphaToOne = false;
    bool MultiViewport = false;
    bool VariableMultisampleRate = false;

    // Texture compression
    bool TextureCompressionETC2 = false;
    bool TextureCompressionASTC_LDR = false;
    bool TextureCompressionBC = false;

    // Sampler
    bool SamplerAnisotropy = false;

    // Image/Texture
    bool ImageCubeArray = false;

    // Shader storage
    bool VertexPipelineStoresAndAtomics = false;
    bool FragmentStoresAndAtomics = false;
    bool ShaderStorageImageExtendedFormats = false;
    bool ShaderStorageImageMultisample = false;
    bool ShaderStorageImageReadWithoutFormat = false;
    bool ShaderStorageImageWriteWithoutFormat = false;

    // Indexing
    bool ShaderUniformBufferArrayDynamicIndexing = false;
    bool ShaderSampledImageArrayDynamicIndexing = false;
    bool ShaderStorageBufferArrayDynamicIndexing = false;
    bool ShaderStorageImageArrayDynamicIndexing = false;

    // Other
    bool RobustBufferAccess = false;
    bool FullDrawIndexUint32 = false;
    bool InheritedQueries = false;
    bool OcclusionQueryPrecise = false;
    bool PipelineStatisticsQuery = false;

    // Ray tracing (extensions)
    bool RayTracingPipeline = false;
    bool RayQuery = false;
    bool AccelerationStructure = false;

    // Mesh shaders (extensions)
    bool MeshShader = false;
    bool TaskShader = false;

    // Bindless
    bool DescriptorIndexing = false;
    bool RuntimeDescriptorArray = false;
};

// =================================================================================================
// Device Properties
// =================================================================================================

export struct RHIDeviceProperties {
    uint32 ApiVersion = 0;
    uint32 DriverVersion = 0;
    uint32 VendorID = 0;
    uint32 DeviceID = 0;
    RHIPhysicalDeviceType DeviceType = RHIPhysicalDeviceType::Other;
    std::string DeviceName;

    // Pipeline cache UUID (for cache compatibility)
    uint8 PipelineCacheUUID[16] = {};

    RHIDeviceLimits Limits;
    RHIDeviceFeatures Features;
};

// =================================================================================================
// Memory Properties
// =================================================================================================

export struct RHIMemoryHeap {
    uint64 Size = 0;
    bool DeviceLocal = false;
};

export struct RHIMemoryType {
    bool DeviceLocal = false;
    bool HostVisible = false;
    bool HostCoherent = false;
    bool HostCached = false;
    bool LazilyAllocated = false;
    uint32 HeapIndex = 0;
};

export struct RHIMemoryProperties {
    std::vector<RHIMemoryHeap> Heaps;
    std::vector<RHIMemoryType> Types;
};

// =================================================================================================
// Format Properties
// =================================================================================================

export struct RHIFormatProperties {
    // Features supported for linear tiling
    bool LinearTilingOptimalImage = false;
    bool LinearTilingSampledImage = false;
    bool LinearTilingStorageImage = false;
    bool LinearTilingColorAttachment = false;
    bool LinearTilingDepthStencilAttachment = false;
    bool LinearTilingBlitSrc = false;
    bool LinearTilingBlitDst = false;

    // Features supported for optimal tiling
    bool OptimalTilingSampledImage = false;
    bool OptimalTilingStorageImage = false;
    bool OptimalTilingColorAttachment = false;
    bool OptimalTilingDepthStencilAttachment = false;
    bool OptimalTilingBlitSrc = false;
    bool OptimalTilingBlitDst = false;

    // Features supported for buffers
    bool BufferUniformTexelBuffer = false;
    bool BufferStorageTexelBuffer = false;
    bool BufferVertexBuffer = false;
};

} // namespace iGe
