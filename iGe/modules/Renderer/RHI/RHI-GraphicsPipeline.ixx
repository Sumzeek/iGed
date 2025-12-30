module;
#include "iGeMacro.h"

export module iGe.RHI:RHIGraphicsPipeline;
import :RHIResource;
import :RHIDescriptor;
import iGe.Common;

namespace iGe
{

// =================================================================================================
// Enums
// =================================================================================================

export enum class RHIPrimitiveTopology {
    PointList = 0,
    LineList,
    LineStrip,
    TriangleList,
    TriangleStrip,
    TriangleFan,
    LineListWithAdjacency,
    LineStripWithAdjacency,
    TriangleListWithAdjacency,
    TriangleStripWithAdjacency,
    PatchList,

    Count
};

export enum class RHIPolygonMode { Fill = 0, Line, Point, Count };

export enum class RHICullMode { None = 0, Front, Back, FrontAndBack, Count };

export enum class RHIFrontFace { CounterClockwise = 0, Clockwise, Count };

export enum class RHIBlendFactor {
    Zero = 0,
    One,
    SrcColor,
    OneMinusSrcColor,
    DstColor,
    OneMinusDstColor,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    ConstantColor,
    OneMinusConstantColor,
    ConstantAlpha,
    OneMinusConstantAlpha,
    SrcAlphaSaturate,
    Src1Color,
    OneMinusSrc1Color,
    Src1Alpha,
    OneMinusSrc1Alpha,

    Count
};

export enum class RHIBlendOp { Add = 0, Subtract, ReverseSubtract, Min, Max, Count };

export enum class RHILogicOp {
    Clear = 0,
    And,
    AndReverse,
    Copy,
    AndInverted,
    NoOp,
    Xor,
    Or,
    Nor,
    Equivalent,
    Invert,
    OrReverse,
    CopyInverted,
    OrInverted,
    Nand,
    Set,

    Count
};

export enum class RHIColorComponentFlagBits : uint32 {
    None = 0,

    R = 1 << 0,
    G = 1 << 1,
    B = 1 << 2,
    A = 1 << 3,
    All = R | G | B | A
};

export enum class RHIDynamicState {
    Viewport = 0,
    Scissor,
    LineWidth,
    DepthBias,
    BlendConstants,
    DepthBounds,
    StencilCompareMask,
    StencilWriteMask,
    StencilReference,

    Count
};

export enum class RHIVertexInputRate { Vertex = 0, Instance, Count };

// =================================================================================================
// State Structures
// =================================================================================================

// 1. Vertex Input State
export struct RHIVertexInputBindingDescription {
    uint32 Binding;
    uint32 Stride;
    RHIVertexInputRate InputRate;
};

export struct RHIVertexInputAttributeDescription {
    uint32 Location;
    uint32 Binding;
    RHIFormat Format;
    uint32 Offset;
};

export struct RHIPipelineVertexInputState {
    std::span<const RHIVertexInputBindingDescription> VertexBindingDescriptions = {};
    std::span<const RHIVertexInputAttributeDescription> VertexAttributeDescriptions = {};
};

// 2. Input Assembly State
export struct RHIPipelineInputAssemblyState {
    RHIPrimitiveTopology Topology = RHIPrimitiveTopology::TriangleList;
    bool PrimitiveRestartEnable = false;
};

// 3. Tessellation State
export struct RHIPipelineTessellationState {
    uint32 PatchControlPoints = 0;
};

// 4. Viewport State
export struct RHIViewport {
    float X;
    float Y;
    float Width;
    float Height;
    float MinDepth;
    float MaxDepth;
};

export struct RHIScissor {
    int32 X, Y;
    uint32 Width, Height;
};

export struct RHIRect2D {
    RHIOffset2D Offset;
    RHIExtent2D Extent;
};

export struct RHIPipelineViewportState {
    std::span<const RHIViewport> Viewports = {}; // If empty, dynamic state is assumed
    std::span<const RHIRect2D> Scissors = {};    // If empty, dynamic state is assumed
    uint32 ViewportCount = 1;                    // Used when Viewports is empty (dynamic state)
    uint32 ScissorCount = 1;                     // Used when Scissors is empty (dynamic state)
};

// 5. Rasterization State
export struct RHIPipelineRasterizationState {
    bool DepthClampEnable = false;
    bool RasterizerDiscardEnable = false;
    RHIPolygonMode PolygonMode = RHIPolygonMode::Fill;
    RHICullMode CullMode = RHICullMode::Back;
    RHIFrontFace FrontFace = RHIFrontFace::CounterClockwise;
    bool DepthBiasEnable = false;
    float DepthBiasConstantFactor = 0.0f;
    float DepthBiasClamp = 0.0f;
    float DepthBiasSlopeFactor = 0.0f;
    float LineWidth = 1.0f;
};

// 6. Multisample State
export struct RHIPipelineMultisampleState {
    uint32 RasterizationSamples = 1; // MSAA sample count
    bool SampleShadingEnable = false;
    float MinSampleShading = 1.0f;
    uint32 SampleMask = 0xFFFFFFFF;
    bool AlphaToCoverageEnable = false;
    bool AlphaToOneEnable = false;
};

// 7. Depth Stencil State
export struct RHIStencilOpState {
    RHIStencilOp FailOp = RHIStencilOp::Keep;
    RHIStencilOp PassOp = RHIStencilOp::Keep;
    RHIStencilOp DepthFailOp = RHIStencilOp::Keep;
    RHICompareOp CompareOp = RHICompareOp::Always;
    uint32 CompareMask = 0xFFFFFFFF;
    uint32 WriteMask = 0xFFFFFFFF;
    uint32 Reference = 0;
};

export struct RHIPipelineDepthStencilState {
    bool DepthTestEnable = true;
    bool DepthWriteEnable = true;
    RHICompareOp DepthCompareOp = RHICompareOp::Less;
    bool DepthBoundsTestEnable = false;
    bool StencilTestEnable = false;
    RHIStencilOpState Front = {};
    RHIStencilOpState Back = {};
    float MinDepthBounds = 0.0f;
    float MaxDepthBounds = 1.0f;
};

// 8. Color Blend State
export struct RHIPipelineColorBlendAttachmentState {
    bool BlendEnable = false;
    RHIBlendFactor SrcColorBlendFactor = RHIBlendFactor::One;
    RHIBlendFactor DstColorBlendFactor = RHIBlendFactor::Zero;
    RHIBlendOp ColorBlendOp = RHIBlendOp::Add;
    RHIBlendFactor SrcAlphaBlendFactor = RHIBlendFactor::One;
    RHIBlendFactor DstAlphaBlendFactor = RHIBlendFactor::Zero;
    RHIBlendOp AlphaBlendOp = RHIBlendOp::Add;
    Flags<RHIColorComponentFlagBits> ColorWriteMask = RHIColorComponentFlagBits::All;
};

export struct RHIPipelineColorBlendState {
    bool LogicOpEnable = false;
    RHILogicOp LogicOp = RHILogicOp::Copy;
    std::span<const RHIPipelineColorBlendAttachmentState> Attachments = {};
    float BlendConstants[4] = {0.0f, 0.0f, 0.0f, 0.0f};
};

// 9. Dynamic State
export struct RHIPipelineDynamicState {
    std::span<const RHIDynamicState> DynamicStates = {};
};

export class RHIGraphicsPipeline;

// =================================================================================================
// Specialization Constants
// =================================================================================================

export struct RHISpecializationMapEntry {
    uint32 ConstantID = 0;
    uint32 Offset = 0;
    uint64 Size = 0;
};

export struct RHISpecializationInfo {
    std::span<const RHISpecializationMapEntry> MapEntries = {};
    std::span<const std::byte> Data = {};
};

// =================================================================================================
// Shader Stage Info (for pipeline creation)
// =================================================================================================

export struct RHIPipelineShaderStageCreateInfo {
    const RHIShader* pShader = nullptr;
    std::string EntryPoint = "main";
    RHISpecializationInfo SpecializationInfo;
};

// =================================================================================================
// Pipeline Info
// =================================================================================================

export struct RHIGraphicsPipelineCreateInfo {
    // Shader Stages
    const RHIShader* pVertexShader = nullptr;
    const RHIShader* pFragmentShader = nullptr;
    const RHIShader* pGeometryShader = nullptr;
    const RHIShader* pTessControlShader = nullptr;
    const RHIShader* pTessEvaluationShader = nullptr;

    // Fixed Function States
    RHIPipelineVertexInputState VertexInputState;
    RHIPipelineInputAssemblyState InputAssemblyState;
    RHIPipelineTessellationState TessellationState;
    RHIPipelineViewportState ViewportState;
    RHIPipelineRasterizationState RasterizationState;
    RHIPipelineMultisampleState MultisampleState;
    RHIPipelineDepthStencilState DepthStencilState;
    RHIPipelineColorBlendState ColorBlendState;
    RHIPipelineDynamicState DynamicState;

    // Pipeline Layout (defines descriptor sets and push constants)
    const RHIPipelineLayout* pLayout;

    // Render Pass / Output
    const RHIRenderPass* pRenderPass;
    uint32 SubpassIndex = 0;

    // Base pipeline for derivatives
    const RHIGraphicsPipeline* pBasePipeline;
    int32 BasePipelineIndex = -1;
};

export class IGE_API RHIGraphicsPipeline : public RHIResource {
public:
    ~RHIGraphicsPipeline() override = default;

protected:
    RHIGraphicsPipeline(const RHIGraphicsPipelineCreateInfo& info) : RHIResource(RHIResourceType::GraphicsPipeline) {}
};

} // namespace iGe
