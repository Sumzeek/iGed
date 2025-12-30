module;
#if defined(IGE_PLATFORM_WINDOWS)
    #include <d3d12.h>
    #include <vector>
    #include <wrl/client.h>

module iGe.RHI;
import :DirectX12GraphicsPipeline;
import :DirectX12Shader;
import :DirectX12Descriptor;
import :DirectX12RenderPass;
import :DirectX12Helper;

namespace iGe
{

D3D12_BLEND ConvertBlendFactor(RHIBlendFactor factor) {
    switch (factor) {
        case RHIBlendFactor::Zero:
            return D3D12_BLEND_ZERO;
        case RHIBlendFactor::One:
            return D3D12_BLEND_ONE;
        case RHIBlendFactor::SrcColor:
            return D3D12_BLEND_SRC_COLOR;
        case RHIBlendFactor::OneMinusSrcColor:
            return D3D12_BLEND_INV_SRC_COLOR;
        case RHIBlendFactor::DstColor:
            return D3D12_BLEND_DEST_COLOR;
        case RHIBlendFactor::OneMinusDstColor:
            return D3D12_BLEND_INV_DEST_COLOR;
        case RHIBlendFactor::SrcAlpha:
            return D3D12_BLEND_SRC_ALPHA;
        case RHIBlendFactor::OneMinusSrcAlpha:
            return D3D12_BLEND_INV_SRC_ALPHA;
        case RHIBlendFactor::DstAlpha:
            return D3D12_BLEND_DEST_ALPHA;
        case RHIBlendFactor::OneMinusDstAlpha:
            return D3D12_BLEND_INV_DEST_ALPHA;
        case RHIBlendFactor::ConstantColor:
            return D3D12_BLEND_BLEND_FACTOR;
        case RHIBlendFactor::OneMinusConstantColor:
            return D3D12_BLEND_INV_BLEND_FACTOR;
        case RHIBlendFactor::ConstantAlpha:
            return D3D12_BLEND_BLEND_FACTOR;
        case RHIBlendFactor::OneMinusConstantAlpha:
            return D3D12_BLEND_INV_BLEND_FACTOR;
        case RHIBlendFactor::SrcAlphaSaturate:
            return D3D12_BLEND_SRC_ALPHA_SAT;
        case RHIBlendFactor::Src1Color:
            return D3D12_BLEND_SRC1_COLOR;
        case RHIBlendFactor::OneMinusSrc1Color:
            return D3D12_BLEND_INV_SRC1_COLOR;
        case RHIBlendFactor::Src1Alpha:
            return D3D12_BLEND_SRC1_ALPHA;
        case RHIBlendFactor::OneMinusSrc1Alpha:
            return D3D12_BLEND_INV_SRC1_ALPHA;
        default:
            return D3D12_BLEND_ONE;
    }
}

D3D12_BLEND_OP ConvertBlendOp(RHIBlendOp op) {
    switch (op) {
        case RHIBlendOp::Add:
            return D3D12_BLEND_OP_ADD;
        case RHIBlendOp::Subtract:
            return D3D12_BLEND_OP_SUBTRACT;
        case RHIBlendOp::ReverseSubtract:
            return D3D12_BLEND_OP_REV_SUBTRACT;
        case RHIBlendOp::Min:
            return D3D12_BLEND_OP_MIN;
        case RHIBlendOp::Max:
            return D3D12_BLEND_OP_MAX;
        default:
            return D3D12_BLEND_OP_ADD;
    }
}

D3D12_COMPARISON_FUNC ConvertCompareOp(RHICompareOp op) {
    switch (op) {
        case RHICompareOp::Never:
            return D3D12_COMPARISON_FUNC_NEVER;
        case RHICompareOp::Less:
            return D3D12_COMPARISON_FUNC_LESS;
        case RHICompareOp::Equal:
            return D3D12_COMPARISON_FUNC_EQUAL;
        case RHICompareOp::LessOrEqual:
            return D3D12_COMPARISON_FUNC_LESS_EQUAL;
        case RHICompareOp::Greater:
            return D3D12_COMPARISON_FUNC_GREATER;
        case RHICompareOp::NotEqual:
            return D3D12_COMPARISON_FUNC_NOT_EQUAL;
        case RHICompareOp::GreaterOrEqual:
            return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
        case RHICompareOp::Always:
            return D3D12_COMPARISON_FUNC_ALWAYS;
        default:
            return D3D12_COMPARISON_FUNC_LESS;
    }
}

D3D12_STENCIL_OP ConvertStencilOp(RHIStencilOp op) {
    switch (op) {
        case RHIStencilOp::Keep:
            return D3D12_STENCIL_OP_KEEP;
        case RHIStencilOp::Zero:
            return D3D12_STENCIL_OP_ZERO;
        case RHIStencilOp::Replace:
            return D3D12_STENCIL_OP_REPLACE;
        case RHIStencilOp::IncrementAndClamp:
            return D3D12_STENCIL_OP_INCR_SAT;
        case RHIStencilOp::DecrementAndClamp:
            return D3D12_STENCIL_OP_DECR_SAT;
        case RHIStencilOp::Invert:
            return D3D12_STENCIL_OP_INVERT;
        case RHIStencilOp::IncrementAndWrap:
            return D3D12_STENCIL_OP_INCR;
        case RHIStencilOp::DecrementAndWrap:
            return D3D12_STENCIL_OP_DECR;
        default:
            return D3D12_STENCIL_OP_KEEP;
    }
}

D3D12_FILL_MODE ConvertPolygonMode(RHIPolygonMode mode) {
    switch (mode) {
        case RHIPolygonMode::Fill:
            return D3D12_FILL_MODE_SOLID;
        case RHIPolygonMode::Line:
            return D3D12_FILL_MODE_WIREFRAME;
        case RHIPolygonMode::Point:
            return D3D12_FILL_MODE_WIREFRAME; // D3D12 doesn't have point mode
        default:
            return D3D12_FILL_MODE_SOLID;
    }
}

D3D12_CULL_MODE ConvertCullMode(RHICullMode mode) {
    switch (mode) {
        case RHICullMode::None:
            return D3D12_CULL_MODE_NONE;
        case RHICullMode::Front:
            return D3D12_CULL_MODE_FRONT;
        case RHICullMode::Back:
            return D3D12_CULL_MODE_BACK;
        default:
            return D3D12_CULL_MODE_NONE;
    }
}

D3D_PRIMITIVE_TOPOLOGY ConvertPrimitiveTopology(RHIPrimitiveTopology topology) {
    switch (topology) {
        case RHIPrimitiveTopology::PointList:
            return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
        case RHIPrimitiveTopology::LineList:
            return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
        case RHIPrimitiveTopology::LineStrip:
            return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
        case RHIPrimitiveTopology::TriangleList:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        case RHIPrimitiveTopology::TriangleStrip:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
        case RHIPrimitiveTopology::TriangleFan:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; // Not supported
        case RHIPrimitiveTopology::LineListWithAdjacency:
            return D3D_PRIMITIVE_TOPOLOGY_LINELIST_ADJ;
        case RHIPrimitiveTopology::LineStripWithAdjacency:
            return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP_ADJ;
        case RHIPrimitiveTopology::TriangleListWithAdjacency:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST_ADJ;
        case RHIPrimitiveTopology::TriangleStripWithAdjacency:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP_ADJ;
        case RHIPrimitiveTopology::PatchList:
            return D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST;
        default:
            return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    }
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE
ConvertPrimitiveTopologyType(RHIPrimitiveTopology topology) {
    switch (topology) {
        case RHIPrimitiveTopology::PointList:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        case RHIPrimitiveTopology::LineList:
        case RHIPrimitiveTopology::LineStrip:
        case RHIPrimitiveTopology::LineListWithAdjacency:
        case RHIPrimitiveTopology::LineStripWithAdjacency:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        case RHIPrimitiveTopology::TriangleList:
        case RHIPrimitiveTopology::TriangleStrip:
        case RHIPrimitiveTopology::TriangleFan:
        case RHIPrimitiveTopology::TriangleListWithAdjacency:
        case RHIPrimitiveTopology::TriangleStripWithAdjacency:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        case RHIPrimitiveTopology::PatchList:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
        default:
            return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    }
}

DXGI_FORMAT ConvertVertexFormat(RHIFormat format) { return RHIFormatToDXGIFormat(format); }

D3D12_INPUT_CLASSIFICATION ConvertInputRate(RHIVertexInputRate rate) {
    switch (rate) {
        case RHIVertexInputRate::Vertex:
            return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        case RHIVertexInputRate::Instance:
            return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
        default:
            return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    }
}

// =================================================================================================
// DirectX12GraphicsPipeline
// =================================================================================================

DirectX12GraphicsPipeline::DirectX12GraphicsPipeline(ID3D12Device* device, const RHIGraphicsPipelineCreateInfo& info)
    : RHIGraphicsPipeline(info) {
    // Get root signature from pipeline layout
    if (info.pLayout) {
        auto* dxLayout = static_cast<const DirectX12PipelineLayout*>(info.pLayout);
        m_RootSignature = dxLayout->GetRootSignature();
    }

    // If no root signature provided, create one based on shader reflection
    if (!m_RootSignature) {
        Internal::LogWarn("No pipeline layout provided, creating root signature from shader reflection");

        std::vector<D3D12_ROOT_PARAMETER> rootParameters;
        std::vector<D3D12_DESCRIPTOR_RANGE> descriptorRanges;

        // Collect resources from all shaders
        auto collectShaderResources = [&](const RHIShader* shader) {
            if (!shader) return;
            auto* dxShader = static_cast<const DirectX12Shader*>(shader);
            const auto& resources = dxShader->GetResources();

            for (const auto& res: resources) {
                D3D12_DESCRIPTOR_RANGE range = {};
                range.NumDescriptors = res.Count;
                range.BaseShaderRegister = res.Register;
                range.RegisterSpace = res.Space;
                range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

                // Determine range type based on resource type (D3D_SHADER_INPUT_TYPE)
                switch (res.Type) {
                    case D3D_SIT_CBUFFER:
                        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                        break;
                    case D3D_SIT_TEXTURE:
                    case D3D_SIT_STRUCTURED:
                    case D3D_SIT_BYTEADDRESS:
                        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                        break;
                    case D3D_SIT_UAV_RWTYPED:
                    case D3D_SIT_UAV_RWSTRUCTURED:
                    case D3D_SIT_UAV_RWBYTEADDRESS:
                    case D3D_SIT_UAV_APPEND_STRUCTURED:
                    case D3D_SIT_UAV_CONSUME_STRUCTURED:
                    case D3D_SIT_UAV_RWSTRUCTURED_WITH_COUNTER:
                        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                        break;
                    case D3D_SIT_SAMPLER:
                        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                        break;
                    default:
                        range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                        break;
                }

                // Check if we already have this range (avoid duplicates)
                bool exists = false;
                for (const auto& existing: descriptorRanges) {
                    if (existing.RangeType == range.RangeType &&
                        existing.BaseShaderRegister == range.BaseShaderRegister &&
                        existing.RegisterSpace == range.RegisterSpace) {
                        exists = true;
                        break;
                    }
                }
                if (!exists) { descriptorRanges.push_back(range); }
            }
        };

        collectShaderResources(info.pVertexShader);
        collectShaderResources(info.pFragmentShader);
        collectShaderResources(info.pGeometryShader);
        collectShaderResources(info.pTessControlShader);
        collectShaderResources(info.pTessEvaluationShader);

        // Group ranges by type
        std::vector<D3D12_DESCRIPTOR_RANGE> cbvRanges, srvRanges, uavRanges, samplerRanges;
        for (const auto& range: descriptorRanges) {
            switch (range.RangeType) {
                case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
                    cbvRanges.push_back(range);
                    break;
                case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
                    srvRanges.push_back(range);
                    break;
                case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
                    uavRanges.push_back(range);
                    break;
                case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
                    samplerRanges.push_back(range);
                    break;
            }
        }

        // Store ranges in member to keep them alive
        std::vector<std::vector<D3D12_DESCRIPTOR_RANGE>> rangeStorage;

        // Combine CBV, SRV, UAV into a single descriptor table for CBV_SRV_UAV heap
        // This matches the BindDescriptorSets logic which binds a single CBV_SRV_UAV table
        std::vector<D3D12_DESCRIPTOR_RANGE> cbvSrvUavRanges;
        cbvSrvUavRanges.insert(cbvSrvUavRanges.end(), cbvRanges.begin(), cbvRanges.end());
        cbvSrvUavRanges.insert(cbvSrvUavRanges.end(), srvRanges.begin(), srvRanges.end());
        cbvSrvUavRanges.insert(cbvSrvUavRanges.end(), uavRanges.begin(), uavRanges.end());

        // Add CBV/SRV/UAV descriptor table as parameter 0
        if (!cbvSrvUavRanges.empty()) {
            rangeStorage.push_back(std::move(cbvSrvUavRanges));
            D3D12_ROOT_PARAMETER param = {};
            param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(rangeStorage.back().size());
            param.DescriptorTable.pDescriptorRanges = rangeStorage.back().data();
            param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            rootParameters.push_back(param);
        }

        // Add Sampler descriptor table as parameter 1 (if any)
        if (!samplerRanges.empty()) {
            rangeStorage.push_back(std::move(samplerRanges));
            D3D12_ROOT_PARAMETER param = {};
            param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param.DescriptorTable.NumDescriptorRanges = static_cast<UINT>(rangeStorage.back().size());
            param.DescriptorTable.pDescriptorRanges = rangeStorage.back().data();
            param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            rootParameters.push_back(param);
        }

        D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
        rootSigDesc.NumParameters = static_cast<UINT>(rootParameters.size());
        rootSigDesc.pParameters = rootParameters.empty() ? nullptr : rootParameters.data();
        rootSigDesc.NumStaticSamplers = 0;
        rootSigDesc.pStaticSamplers = nullptr;
        rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

        HRESULT hr =
                D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signatureBlob, &errorBlob);
        if (FAILED(hr)) {
            std::string errorMsg = "Failed to serialize default root signature";
            if (errorBlob) {
                errorMsg += ": ";
                errorMsg += static_cast<const char*>(errorBlob->GetBufferPointer());
            }
            Internal::LogError("{}", errorMsg);
            return;
        }

        hr = device->CreateRootSignature(0, signatureBlob->GetBufferPointer(), signatureBlob->GetBufferSize(),
                                         IID_PPV_ARGS(&m_RootSignature));
        if (FAILED(hr)) {
            Internal::LogError("Failed to create default root signature: {}", hr);
            return;
        }
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = m_RootSignature.Get();

    // Helper function to set shader bytecode
    auto setShaderBytecode = [](D3D12_SHADER_BYTECODE& target, const RHIShader* shader) {
        if (!shader) return;
        auto* dxShader = static_cast<const DirectX12Shader*>(shader);
        target.pShaderBytecode = dxShader->GetBytecode();
        target.BytecodeLength = dxShader->GetBytecodeSize();
    };

    // Set shaders from direct references
    if (info.pVertexShader) { setShaderBytecode(psoDesc.VS, info.pVertexShader); }
    if (info.pFragmentShader) { setShaderBytecode(psoDesc.PS, info.pFragmentShader); }
    if (info.pGeometryShader) { setShaderBytecode(psoDesc.GS, info.pGeometryShader); }
    if (info.pTessControlShader) { setShaderBytecode(psoDesc.HS, info.pTessControlShader); }
    if (info.pTessEvaluationShader) { setShaderBytecode(psoDesc.DS, info.pTessEvaluationShader); }

    // Input layout - use shader reflection data if available
    std::vector<D3D12_INPUT_ELEMENT_DESC> inputElements;
    std::vector<std::string> semanticNameStorage; // Keep semantic name strings alive

    // Try to get input layout from vertex shader reflection
    const DirectX12Shader* dxVertexShader = nullptr;
    if (info.pVertexShader) { dxVertexShader = static_cast<const DirectX12Shader*>(info.pVertexShader); }

    if (dxVertexShader && !dxVertexShader->GetInputLayout().empty()) {
        // Use shader reflection for input layout (preferred method)
        const auto& shaderInputLayout = dxVertexShader->GetInputLayout();
        semanticNameStorage.reserve(shaderInputLayout.size());

        for (const auto& shaderElem: shaderInputLayout) {
            // Store semantic name to keep it alive
            semanticNameStorage.push_back(shaderElem.SemanticName);

            D3D12_INPUT_ELEMENT_DESC elem = {};
            elem.SemanticName = semanticNameStorage.back().c_str();
            elem.SemanticIndex = shaderElem.SemanticIndex;
            elem.Format = shaderElem.Format;
            elem.InputSlot = shaderElem.InputSlot;
            elem.AlignedByteOffset = shaderElem.AlignedByteOffset;
            elem.InputSlotClass = shaderElem.InputSlotClass;
            elem.InstanceDataStepRate = shaderElem.InstanceDataStepRate;

            // Override with user-provided binding info if available
            for (const auto& binding: info.VertexInputState.VertexBindingDescriptions) {
                if (binding.Binding == shaderElem.InputSlot) {
                    elem.InputSlotClass = ConvertInputRate(binding.InputRate);
                    elem.InstanceDataStepRate = (binding.InputRate == RHIVertexInputRate::Instance) ? 1 : 0;
                    break;
                }
            }

            // Override offset from user-provided attribute info if available
            for (const auto& attribute: info.VertexInputState.VertexAttributeDescriptions) {
                // Match by location (semantic index for TEXCOORD-like semantics)
                if (attribute.Location == inputElements.size()) {
                    elem.AlignedByteOffset = attribute.Offset;
                    elem.InputSlot = attribute.Binding;
                    if (elem.Format == DXGI_FORMAT_UNKNOWN) { elem.Format = ConvertVertexFormat(attribute.Format); }
                    break;
                }
            }

            inputElements.push_back(elem);
        }
    } else {
        // Fallback: build input layout from user-provided vertex input state
        // Use common semantic name mapping based on location
        auto getSemanticForLocation = [](uint32 location) -> std::pair<std::string, uint32> {
            // Common convention: location 0 = POSITION, 1 = NORMAL, 2 = TEXCOORD, etc.
            switch (location) {
                case 0:
                    return {"POSITION", 0};
                case 1:
                    return {"NORMAL", 0};
                case 2:
                    return {"TEXCOORD", 0};
                case 3:
                    return {"TEXCOORD", 1};
                case 4:
                    return {"COLOR", 0};
                case 5:
                    return {"TANGENT", 0};
                case 6:
                    return {"BINORMAL", 0};
                default:
                    return {"TEXCOORD", location - 2};
            }
        };

        for (const auto& attribute: info.VertexInputState.VertexAttributeDescriptions) {
            auto [semanticName, semanticIndex] = getSemanticForLocation(attribute.Location);
            semanticNameStorage.push_back(semanticName);
            D3D12_INPUT_ELEMENT_DESC elem = {};
            elem.SemanticName = semanticNameStorage.back().c_str();
            elem.SemanticIndex = semanticIndex;
            elem.Format = ConvertVertexFormat(attribute.Format);
            elem.InputSlot = attribute.Binding;
            elem.AlignedByteOffset = attribute.Offset;
            elem.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
            elem.InstanceDataStepRate = 0;

            // Get input rate from binding descriptions
            for (const auto& binding: info.VertexInputState.VertexBindingDescriptions) {
                if (binding.Binding == attribute.Binding) {
                    elem.InputSlotClass = ConvertInputRate(binding.InputRate);
                    elem.InstanceDataStepRate = (binding.InputRate == RHIVertexInputRate::Instance) ? 1 : 0;
                    break;
                }
            }

            inputElements.push_back(elem);
        }
    }

    psoDesc.InputLayout.pInputElementDescs = inputElements.data();
    psoDesc.InputLayout.NumElements = static_cast<UINT>(inputElements.size());

    // Rasterizer state
    psoDesc.RasterizerState.FillMode = ConvertPolygonMode(info.RasterizationState.PolygonMode);
    psoDesc.RasterizerState.CullMode = ConvertCullMode(info.RasterizationState.CullMode);
    psoDesc.RasterizerState.FrontCounterClockwise =
            (info.RasterizationState.FrontFace == RHIFrontFace::CounterClockwise);
    psoDesc.RasterizerState.DepthBias = static_cast<INT>(info.RasterizationState.DepthBiasConstantFactor);
    psoDesc.RasterizerState.DepthBiasClamp = info.RasterizationState.DepthBiasClamp;
    psoDesc.RasterizerState.SlopeScaledDepthBias = info.RasterizationState.DepthBiasSlopeFactor;
    psoDesc.RasterizerState.DepthClipEnable = info.RasterizationState.DepthClampEnable ? FALSE : TRUE;
    psoDesc.RasterizerState.MultisampleEnable = info.MultisampleState.RasterizationSamples > 1;
    psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
    psoDesc.RasterizerState.ForcedSampleCount = 0;
    psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // Depth stencil state
    psoDesc.DepthStencilState.DepthEnable = info.DepthStencilState.DepthTestEnable;
    psoDesc.DepthStencilState.DepthWriteMask =
            info.DepthStencilState.DepthWriteEnable ? D3D12_DEPTH_WRITE_MASK_ALL : D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.DepthFunc = ConvertCompareOp(info.DepthStencilState.DepthCompareOp);
    psoDesc.DepthStencilState.StencilEnable = info.DepthStencilState.StencilTestEnable;
    psoDesc.DepthStencilState.StencilReadMask = static_cast<UINT8>(info.DepthStencilState.Front.CompareMask);
    psoDesc.DepthStencilState.StencilWriteMask = static_cast<UINT8>(info.DepthStencilState.Front.WriteMask);

    // Front face stencil
    psoDesc.DepthStencilState.FrontFace.StencilFailOp = ConvertStencilOp(info.DepthStencilState.Front.FailOp);
    psoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = ConvertStencilOp(info.DepthStencilState.Front.DepthFailOp);
    psoDesc.DepthStencilState.FrontFace.StencilPassOp = ConvertStencilOp(info.DepthStencilState.Front.PassOp);
    psoDesc.DepthStencilState.FrontFace.StencilFunc = ConvertCompareOp(info.DepthStencilState.Front.CompareOp);

    // Back face stencil
    psoDesc.DepthStencilState.BackFace.StencilFailOp = ConvertStencilOp(info.DepthStencilState.Back.FailOp);
    psoDesc.DepthStencilState.BackFace.StencilDepthFailOp = ConvertStencilOp(info.DepthStencilState.Back.DepthFailOp);
    psoDesc.DepthStencilState.BackFace.StencilPassOp = ConvertStencilOp(info.DepthStencilState.Back.PassOp);
    psoDesc.DepthStencilState.BackFace.StencilFunc = ConvertCompareOp(info.DepthStencilState.Back.CompareOp);

    // Blend state
    psoDesc.BlendState.AlphaToCoverageEnable = info.MultisampleState.AlphaToCoverageEnable;
    psoDesc.BlendState.IndependentBlendEnable = TRUE;

    const uint32 blendAttachmentCount = static_cast<uint32>(info.ColorBlendState.Attachments.size());
    for (uint32 i = 0; i < blendAttachmentCount && i < 8; ++i) {
        const auto& attachment = info.ColorBlendState.Attachments[i];
        auto& rtBlend = psoDesc.BlendState.RenderTarget[i];

        rtBlend.BlendEnable = attachment.BlendEnable;
        rtBlend.LogicOpEnable = info.ColorBlendState.LogicOpEnable;
        rtBlend.SrcBlend = ConvertBlendFactor(attachment.SrcColorBlendFactor);
        rtBlend.DestBlend = ConvertBlendFactor(attachment.DstColorBlendFactor);
        rtBlend.BlendOp = ConvertBlendOp(attachment.ColorBlendOp);
        rtBlend.SrcBlendAlpha = ConvertBlendFactor(attachment.SrcAlphaBlendFactor);
        rtBlend.DestBlendAlpha = ConvertBlendFactor(attachment.DstAlphaBlendFactor);
        rtBlend.BlendOpAlpha = ConvertBlendOp(attachment.AlphaBlendOp);
        rtBlend.RenderTargetWriteMask = static_cast<UINT8>(attachment.ColorWriteMask.GetValue());

        // Logic op
        if (info.ColorBlendState.LogicOpEnable) {
            switch (info.ColorBlendState.LogicOp) {
                case RHILogicOp::Clear:
                    rtBlend.LogicOp = D3D12_LOGIC_OP_CLEAR;
                    break;
                case RHILogicOp::And:
                    rtBlend.LogicOp = D3D12_LOGIC_OP_AND;
                    break;
                case RHILogicOp::Or:
                    rtBlend.LogicOp = D3D12_LOGIC_OP_OR;
                    break;
                case RHILogicOp::Xor:
                    rtBlend.LogicOp = D3D12_LOGIC_OP_XOR;
                    break;
                case RHILogicOp::Copy:
                    rtBlend.LogicOp = D3D12_LOGIC_OP_COPY;
                    break;
                case RHILogicOp::NoOp:
                    rtBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
                    break;
                default:
                    rtBlend.LogicOp = D3D12_LOGIC_OP_NOOP;
                    break;
            }
        }
    }

    // Render target formats
    if (info.pRenderPass) {
        auto* dxRenderPass = static_cast<const DirectX12RenderPass*>(info.pRenderPass);

        // Get RTV formats for the specified subpass
        std::vector<DXGI_FORMAT> rtvFormats = dxRenderPass->GetRTVFormats(info.SubpassIndex);
        psoDesc.NumRenderTargets = static_cast<UINT>(rtvFormats.size());

        for (size_t i = 0; i < rtvFormats.size() && i < 8; ++i) { psoDesc.RTVFormats[i] = rtvFormats[i]; }

        // Get DSV format for the specified subpass
        DXGI_FORMAT dsvFormat = dxRenderPass->GetDSVFormat(info.SubpassIndex);
        if (dsvFormat != DXGI_FORMAT_UNKNOWN) { psoDesc.DSVFormat = dsvFormat; }
    } else {
        // Default format if no render pass
        psoDesc.NumRenderTargets = 1;
        psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    }

    // Sample desc
    psoDesc.SampleDesc.Count = info.MultisampleState.RasterizationSamples;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.SampleMask = info.MultisampleState.SampleMask;

    // Primitive topology type
    psoDesc.PrimitiveTopologyType = ConvertPrimitiveTopologyType(info.InputAssemblyState.Topology);
    m_PrimitiveTopology = ConvertPrimitiveTopology(info.InputAssemblyState.Topology);

    // Create the PSO
    HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_PipelineState));
    if (FAILED(hr)) { Internal::LogError("Failed to create graphics pipeline state: {}", hr); }
}

DirectX12GraphicsPipeline::~DirectX12GraphicsPipeline() {}

} // namespace iGe
#endif
