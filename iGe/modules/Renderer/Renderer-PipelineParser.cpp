module;
#include <fstream>
#include <nlohmann/json.hpp>

module iGe.Renderer;
import :PipelineParser;

namespace iGe
{

// =================================================================================================
// Helper macros for enum conversion
// =================================================================================================

#define JSON_TO_ENUM(JsonVal, EnumType, EnumVal)                                                                       \
    if (JsonVal == #EnumVal) return EnumType::EnumVal;

#define BEGIN_ENUM_MAP(EnumType) static EnumType StringTo##EnumType(const std::string& str) {

#define END_ENUM_MAP(DefaultVal)                                                                                       \
    return DefaultVal;                                                                                                 \
    }

// =================================================================================================
// Enum conversions
// =================================================================================================

BEGIN_ENUM_MAP(RHIPrimitiveTopology)
JSON_TO_ENUM(str, RHIPrimitiveTopology, PointList)
JSON_TO_ENUM(str, RHIPrimitiveTopology, LineList)
JSON_TO_ENUM(str, RHIPrimitiveTopology, LineStrip)
JSON_TO_ENUM(str, RHIPrimitiveTopology, TriangleList)
JSON_TO_ENUM(str, RHIPrimitiveTopology, TriangleStrip)
JSON_TO_ENUM(str, RHIPrimitiveTopology, TriangleFan)
END_ENUM_MAP(RHIPrimitiveTopology::TriangleList)

BEGIN_ENUM_MAP(RHIPolygonMode)
JSON_TO_ENUM(str, RHIPolygonMode, Fill)
JSON_TO_ENUM(str, RHIPolygonMode, Line)
JSON_TO_ENUM(str, RHIPolygonMode, Point)
END_ENUM_MAP(RHIPolygonMode::Fill)

BEGIN_ENUM_MAP(RHICullMode)
JSON_TO_ENUM(str, RHICullMode, None)
JSON_TO_ENUM(str, RHICullMode, Front)
JSON_TO_ENUM(str, RHICullMode, Back)
JSON_TO_ENUM(str, RHICullMode, FrontAndBack)
END_ENUM_MAP(RHICullMode::Back)

BEGIN_ENUM_MAP(RHIFrontFace)
JSON_TO_ENUM(str, RHIFrontFace, CounterClockwise)
JSON_TO_ENUM(str, RHIFrontFace, Clockwise)
END_ENUM_MAP(RHIFrontFace::CounterClockwise)

BEGIN_ENUM_MAP(RHICompareOp)
JSON_TO_ENUM(str, RHICompareOp, Never)
JSON_TO_ENUM(str, RHICompareOp, Less)
JSON_TO_ENUM(str, RHICompareOp, Equal)
JSON_TO_ENUM(str, RHICompareOp, LessOrEqual)
JSON_TO_ENUM(str, RHICompareOp, Greater)
JSON_TO_ENUM(str, RHICompareOp, NotEqual)
JSON_TO_ENUM(str, RHICompareOp, GreaterOrEqual)
JSON_TO_ENUM(str, RHICompareOp, Always)
END_ENUM_MAP(RHICompareOp::Less)

BEGIN_ENUM_MAP(RHIStencilOp)
JSON_TO_ENUM(str, RHIStencilOp, Keep)
JSON_TO_ENUM(str, RHIStencilOp, Zero)
JSON_TO_ENUM(str, RHIStencilOp, Replace)
JSON_TO_ENUM(str, RHIStencilOp, IncrementAndClamp)
JSON_TO_ENUM(str, RHIStencilOp, DecrementAndClamp)
JSON_TO_ENUM(str, RHIStencilOp, Invert)
JSON_TO_ENUM(str, RHIStencilOp, IncrementAndWrap)
JSON_TO_ENUM(str, RHIStencilOp, DecrementAndWrap)
END_ENUM_MAP(RHIStencilOp::Keep)

BEGIN_ENUM_MAP(RHIBlendFactor)
JSON_TO_ENUM(str, RHIBlendFactor, Zero)
JSON_TO_ENUM(str, RHIBlendFactor, One)
JSON_TO_ENUM(str, RHIBlendFactor, SrcColor)
JSON_TO_ENUM(str, RHIBlendFactor, OneMinusSrcColor)
JSON_TO_ENUM(str, RHIBlendFactor, DstColor)
JSON_TO_ENUM(str, RHIBlendFactor, OneMinusDstColor)
JSON_TO_ENUM(str, RHIBlendFactor, SrcAlpha)
JSON_TO_ENUM(str, RHIBlendFactor, OneMinusSrcAlpha)
JSON_TO_ENUM(str, RHIBlendFactor, DstAlpha)
JSON_TO_ENUM(str, RHIBlendFactor, OneMinusDstAlpha)
END_ENUM_MAP(RHIBlendFactor::One)

BEGIN_ENUM_MAP(RHIBlendOp)
JSON_TO_ENUM(str, RHIBlendOp, Add)
JSON_TO_ENUM(str, RHIBlendOp, Subtract)
JSON_TO_ENUM(str, RHIBlendOp, ReverseSubtract)
JSON_TO_ENUM(str, RHIBlendOp, Min)
JSON_TO_ENUM(str, RHIBlendOp, Max)
END_ENUM_MAP(RHIBlendOp::Add)

BEGIN_ENUM_MAP(RHILogicOp)
JSON_TO_ENUM(str, RHILogicOp, Clear)
JSON_TO_ENUM(str, RHILogicOp, Copy)
JSON_TO_ENUM(str, RHILogicOp, NoOp)
END_ENUM_MAP(RHILogicOp::Copy)

BEGIN_ENUM_MAP(RHIVertexInputRate)
JSON_TO_ENUM(str, RHIVertexInputRate, Vertex)
JSON_TO_ENUM(str, RHIVertexInputRate, Instance)
END_ENUM_MAP(RHIVertexInputRate::Vertex)

BEGIN_ENUM_MAP(RHIFormat)
JSON_TO_ENUM(str, RHIFormat, R32G32B32SFloat)
JSON_TO_ENUM(str, RHIFormat, R32G32SFloat)
JSON_TO_ENUM(str, RHIFormat, R32G32B32A32SFloat)
JSON_TO_ENUM(str, RHIFormat, R8G8B8A8UNorm)
// Add more formats as needed
END_ENUM_MAP(RHIFormat::Unknown)

BEGIN_ENUM_MAP(RHIDynamicState)
JSON_TO_ENUM(str, RHIDynamicState, Viewport)
JSON_TO_ENUM(str, RHIDynamicState, Scissor)
JSON_TO_ENUM(str, RHIDynamicState, LineWidth)
END_ENUM_MAP(RHIDynamicState::Viewport)

// =================================================================================================
// Static Helper Methods
// =================================================================================================

static RHIShaderStage StringToRHIShaderStage(const std::string& str) {
    if (str == "vertex" || str == "Vertex") { return RHIShaderStage::Vertex; }
    if (str == "fragment" || str == "Fragment") { return RHIShaderStage::Fragment; }
    if (str == "geometry" || str == "Geometry") { return RHIShaderStage::Geometry; }
    if (str == "tessControl" || str == "TessControl") { return RHIShaderStage::TessControl; }
    if (str == "tessEvaluation" || str == "TessEvaluation") { return RHIShaderStage::TessEvaluation; }
    if (str == "compute" || str == "Compute") { return RHIShaderStage::Compute; }

    Internal::LogWarn("Unrecognized shader stage string: {}. Defaulting to Vertex stage.", str);
    return RHIShaderStage::Vertex;
}

static Flags<RHIColorComponentFlagBits> ParseColorWriteMask(const nlohmann::json& j) {
    Flags<RHIColorComponentFlagBits> mask = RHIColorComponentFlagBits::All;
    if (j.is_array()) {
        mask.Reset();
        for (const auto& item: j) {
            std::string s = item.get<std::string>();
            if (s == "R") {
                mask.AddFlag(RHIColorComponentFlagBits::R);
            } else if (s == "G") {
                mask.AddFlag(RHIColorComponentFlagBits::G);
            } else if (s == "B") {
                mask.AddFlag(RHIColorComponentFlagBits::B);
            } else if (s == "A") {
                mask.AddFlag(RHIColorComponentFlagBits::A);
            }
        }
    }
    return mask;
}

// =================================================================================================
// Internal ParseInfo Structure (holds ownership of dynamically allocated data)
// =================================================================================================

struct ParsedPipelineData {
    // Shaders (owned)
    Scope<RHIShader> VertexShader;
    Scope<RHIShader> FragmentShader;
    Scope<RHIShader> GeometryShader;
    Scope<RHIShader> TessControlShader;
    Scope<RHIShader> TessEvaluationShader;

    // Vertex Input Data (owned arrays)
    std::vector<RHIVertexInputBindingDescription> VertexBindings;
    std::vector<RHIVertexInputAttributeDescription> VertexAttributes;

    // Color Blend Attachments (owned array)
    std::vector<RHIPipelineColorBlendAttachmentState> ColorBlendAttachments;

    // Dynamic States (owned array)
    std::vector<RHIDynamicState> DynamicStates;

    // Pipeline Create Info
    RHIGraphicsPipelineCreateInfo CreateInfo{};
};

// =================================================================================================
// JSON Parsing Implementation
// =================================================================================================

static void ParseShaders(const nlohmann::json& j, ParsedPipelineData& data, ShaderLoader shaderLoader) {
    if (!j.contains("shaders")) return;

    const auto& shaders = j["shaders"];
    for (const auto& shader: shaders) {
        std::string stageStr = shader.value("stage", "vertex");
        std::string path = shader.value("path", "");
        std::string entryPoint = shader.value("entry", "main");

        if (path.empty()) {
            Internal::LogWarn("PipelineParser: Shader path is empty for stage '{}'", stageStr);
            continue;
        }

        RHIShaderStage stage = StringToRHIShaderStage(stageStr);
        auto loadedShader = shaderLoader(stage, path, entryPoint);

        if (!loadedShader) {
            Internal::LogError("PipelineParser: Failed to load shader: {}", path);
            continue;
        }

        switch (stage) {
            case RHIShaderStage::Vertex:
                data.VertexShader = std::move(loadedShader);
                data.CreateInfo.pVertexShader = data.VertexShader.get();
                break;
            case RHIShaderStage::Fragment:
                data.FragmentShader = std::move(loadedShader);
                data.CreateInfo.pFragmentShader = data.FragmentShader.get();
                break;
            case RHIShaderStage::Geometry:
                data.GeometryShader = std::move(loadedShader);
                data.CreateInfo.pGeometryShader = data.GeometryShader.get();
                break;
            case RHIShaderStage::TessControl:
                data.TessControlShader = std::move(loadedShader);
                data.CreateInfo.pTessControlShader = data.TessControlShader.get();
                break;
            case RHIShaderStage::TessEvaluation:
                data.TessEvaluationShader = std::move(loadedShader);
                data.CreateInfo.pTessEvaluationShader = data.TessEvaluationShader.get();
                break;
            default:
                Internal::LogWarn("PipelineParser: Unsupported shader stage for graphics pipeline");
                break;
        }
    }
}

static void ParseVertexInput(const nlohmann::json& j, ParsedPipelineData& data) {
    if (!j.contains("vertexInput")) return;

    const auto& vi = j["vertexInput"];

    // Parse bindings
    if (vi.contains("bindings")) {
        for (const auto& binding: vi["bindings"]) {
            RHIVertexInputBindingDescription desc{};
            desc.Binding = binding.value("binding", 0u);
            desc.Stride = binding.value("stride", 0u);
            desc.InputRate = StringToRHIVertexInputRate(binding.value("inputRate", "Vertex"));
            data.VertexBindings.push_back(desc);
        }
    }

    // Parse attributes
    if (vi.contains("attributes")) {
        for (const auto& attr: vi["attributes"]) {
            RHIVertexInputAttributeDescription desc{};
            desc.Location = attr.value("location", 0u);
            desc.Binding = attr.value("binding", 0u);
            desc.Format = StringToRHIFormat(attr.value("format", "R32G32B32SFloat"));
            desc.Offset = attr.value("offset", 0u);
            data.VertexAttributes.push_back(desc);
        }
    }

    // Set spans
    data.CreateInfo.VertexInputState.VertexBindingDescriptions = data.VertexBindings;
    data.CreateInfo.VertexInputState.VertexAttributeDescriptions = data.VertexAttributes;
}

static void ParseInputAssembly(const nlohmann::json& j, ParsedPipelineData& data) {
    if (!j.contains("inputAssembly")) return;

    const auto& ia = j["inputAssembly"];
    data.CreateInfo.InputAssemblyState.Topology = StringToRHIPrimitiveTopology(ia.value("topology", "TriangleList"));
    data.CreateInfo.InputAssemblyState.PrimitiveRestartEnable = ia.value("primitiveRestart", false);
}

static void ParseRasterization(const nlohmann::json& j, ParsedPipelineData& data) {
    if (!j.contains("rasterization")) return;

    const auto& rs = j["rasterization"];
    auto& state = data.CreateInfo.RasterizationState;

    state.DepthClampEnable = rs.value("depthClamp", false);
    state.RasterizerDiscardEnable = rs.value("rasterizerDiscard", false);
    state.PolygonMode = StringToRHIPolygonMode(rs.value("polygonMode", "Fill"));
    state.CullMode = StringToRHICullMode(rs.value("cullMode", "Back"));
    state.FrontFace = StringToRHIFrontFace(rs.value("frontFace", "CounterClockwise"));
    state.DepthBiasEnable = rs.value("depthBias", false);
    state.DepthBiasConstantFactor = rs.value("depthBiasConstant", 0.0f);
    state.DepthBiasClamp = rs.value("depthBiasClamp", 0.0f);
    state.DepthBiasSlopeFactor = rs.value("depthBiasSlope", 0.0f);
    state.LineWidth = rs.value("lineWidth", 1.0f);
}

static void ParseDepthStencil(const nlohmann::json& j, ParsedPipelineData& data) {
    if (!j.contains("depthStencil")) return;

    const auto& ds = j["depthStencil"];
    auto& state = data.CreateInfo.DepthStencilState;

    state.DepthTestEnable = ds.value("depthTest", true);
    state.DepthWriteEnable = ds.value("depthWrite", true);
    state.DepthCompareOp = StringToRHICompareOp(ds.value("depthCompare", "Less"));
    state.DepthBoundsTestEnable = ds.value("depthBoundsTest", false);
    state.StencilTestEnable = ds.value("stencilTest", false);
    state.MinDepthBounds = ds.value("minDepthBounds", 0.0f);
    state.MaxDepthBounds = ds.value("maxDepthBounds", 1.0f);

    // Parse front stencil ops
    if (ds.contains("front")) {
        const auto& front = ds["front"];
        state.Front.FailOp = StringToRHIStencilOp(front.value("failOp", "Keep"));
        state.Front.PassOp = StringToRHIStencilOp(front.value("passOp", "Keep"));
        state.Front.DepthFailOp = StringToRHIStencilOp(front.value("depthFailOp", "Keep"));
        state.Front.CompareOp = StringToRHICompareOp(front.value("compareOp", "Always"));
        state.Front.CompareMask = front.value("compareMask", 0xFFFFFFFFu);
        state.Front.WriteMask = front.value("writeMask", 0xFFFFFFFFu);
        state.Front.Reference = front.value("reference", 0u);
    }

    // Parse back stencil ops
    if (ds.contains("back")) {
        const auto& back = ds["back"];
        state.Back.FailOp = StringToRHIStencilOp(back.value("failOp", "Keep"));
        state.Back.PassOp = StringToRHIStencilOp(back.value("passOp", "Keep"));
        state.Back.DepthFailOp = StringToRHIStencilOp(back.value("depthFailOp", "Keep"));
        state.Back.CompareOp = StringToRHICompareOp(back.value("compareOp", "Always"));
        state.Back.CompareMask = back.value("compareMask", 0xFFFFFFFFu);
        state.Back.WriteMask = back.value("writeMask", 0xFFFFFFFFu);
        state.Back.Reference = back.value("reference", 0u);
    }
}

static void ParseColorBlend(const nlohmann::json& j, ParsedPipelineData& data) {
    if (!j.contains("colorBlend")) return;

    const auto& cb = j["colorBlend"];
    auto& state = data.CreateInfo.ColorBlendState;

    state.LogicOpEnable = cb.value("logicOpEnable", false);
    state.LogicOp = StringToRHILogicOp(cb.value("logicOp", "Copy"));

    // Parse blend constants
    if (cb.contains("blendConstants") && cb["blendConstants"].is_array()) {
        const auto& constants = cb["blendConstants"];
        for (size_t i = 0; i < 4 && i < constants.size(); ++i) { state.BlendConstants[i] = constants[i].get<float>(); }
    }

    // Parse attachments
    if (cb.contains("attachments")) {
        for (const auto& attachment: cb["attachments"]) {
            RHIPipelineColorBlendAttachmentState attState{};
            attState.BlendEnable = attachment.value("blendEnable", false);
            attState.SrcColorBlendFactor = StringToRHIBlendFactor(attachment.value("srcColorBlend", "One"));
            attState.DstColorBlendFactor = StringToRHIBlendFactor(attachment.value("dstColorBlend", "Zero"));
            attState.ColorBlendOp = StringToRHIBlendOp(attachment.value("colorBlendOp", "Add"));
            attState.SrcAlphaBlendFactor = StringToRHIBlendFactor(attachment.value("srcAlphaBlend", "One"));
            attState.DstAlphaBlendFactor = StringToRHIBlendFactor(attachment.value("dstAlphaBlend", "Zero"));
            attState.AlphaBlendOp = StringToRHIBlendOp(attachment.value("alphaBlendOp", "Add"));

            if (attachment.contains("colorWriteMask")) {
                attState.ColorWriteMask = ParseColorWriteMask(attachment["colorWriteMask"]);
            }

            data.ColorBlendAttachments.push_back(attState);
        }
    }

    // If no attachments specified, add a default one
    if (data.ColorBlendAttachments.empty()) {
        data.ColorBlendAttachments.push_back(RHIPipelineColorBlendAttachmentState{});
    }

    state.Attachments = data.ColorBlendAttachments;
}

static void ParseMultisample(const nlohmann::json& j, ParsedPipelineData& data) {
    if (!j.contains("multisample")) return;

    const auto& ms = j["multisample"];
    auto& state = data.CreateInfo.MultisampleState;

    state.RasterizationSamples = ms.value("samples", 1u);
    state.SampleShadingEnable = ms.value("sampleShading", false);
    state.MinSampleShading = ms.value("minSampleShading", 1.0f);
    state.SampleMask = ms.value("sampleMask", 0xFFFFFFFFu);
    state.AlphaToCoverageEnable = ms.value("alphaToCoverage", false);
    state.AlphaToOneEnable = ms.value("alphaToOne", false);
}

static void ParseDynamicState(const nlohmann::json& j, ParsedPipelineData& data) {
    if (!j.contains("dynamicStates")) return;

    const auto& ds = j["dynamicStates"];
    if (!ds.is_array()) return;

    for (const auto& state: ds) { data.DynamicStates.push_back(StringToRHIDynamicState(state.get<std::string>())); }

    data.CreateInfo.DynamicState.DynamicStates = data.DynamicStates;
}

static void ParseViewport(const nlohmann::json& j, ParsedPipelineData& data) {
    if (!j.contains("viewport")) return;

    const auto& vp = j["viewport"];
    auto& state = data.CreateInfo.ViewportState;

    state.ViewportCount = vp.value("viewportCount", 1u);
    state.ScissorCount = vp.value("scissorCount", 1u);
}

static ParsedPipelineData ParsePipelineJson(const std::filesystem::path& jsonPath, ShaderLoader shaderLoader,
                                            const RHIRenderPass* pRenderPass,
                                            const RHIPipelineLayout* pPipelineLayout) {

    ParsedPipelineData data{};

    try {
        // Read JSON file content
        if (!std::filesystem::exists(jsonPath)) {
            Internal::LogError("PipelineParser: JSON file not found - {}", jsonPath.string());
            return data;
        }

        std::ifstream file(jsonPath);
        if (!file.is_open()) {
            Internal::LogError("PipelineParser: Failed to open JSON file - {}", jsonPath.string());
            return data;
        }

        nlohmann::json j = nlohmann::json::parse(file);

        // Parse all sections
        ParseShaders(j, data, std::move(shaderLoader));
        ParseVertexInput(j, data);
        ParseInputAssembly(j, data);
        ParseRasterization(j, data);
        ParseDepthStencil(j, data);
        ParseColorBlend(j, data);
        ParseMultisample(j, data);
        ParseDynamicState(j, data);
        ParseViewport(j, data);

        // Set external references
        data.CreateInfo.pRenderPass = pRenderPass;
        data.CreateInfo.pLayout = pPipelineLayout;
        data.CreateInfo.SubpassIndex = j.value("subpassIndex", 0u);

    } catch (const nlohmann::json::exception& e) {
        Internal::LogError("PipelineParser: JSON parse error - {}", e.what());
    }

    return data;
}

// =================================================================================================
// PipelineParser Public Interface Implementation
// =================================================================================================

Scope<RHIGraphicsPipeline> PipelineParser::CreateGraphicsPipeline(const std::filesystem::path& jsonContent,
                                                                  ShaderLoader shaderLoader,
                                                                  const RHIRenderPass* pRenderPass,
                                                                  const RHIPipelineLayout* pPipelineLayout) {

    ParsedPipelineData data = ParsePipelineJson(jsonContent, std::move(shaderLoader), pRenderPass, pPipelineLayout);

    auto rhi = RHI::Get();
    if (!rhi) {
        Internal::LogError("PipelineParser: RHI not initialized");
        return nullptr;
    }

    return rhi->CreateGraphicsPipeline(data.CreateInfo);
}

} // namespace iGe
