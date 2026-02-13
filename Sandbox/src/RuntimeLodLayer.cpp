module;
#include "glad/gl.h"
#include "iGeMacro.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

module iGed.RuntimeLodLayer;
import iGed.MeshBaker;
import iGed.LUT;
import std;
import glm;

/////////////////////////////////////////////////////////////////////////////
// RuntimeLodLayer //////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
RuntimeLodLayer::RuntimeLodLayer()
    : Layer{"RuntimeLod"}, m_Camera{60.0f, 1280.0f / 720.0f, 0.01f, 1000.f}, m_CameraPosition{0.0f} {
    // Create empty VAO
    {
        m_EmptyVertexArray = iGe::VertexArray::Create();
        std::vector<std::uint32_t> indices = {0, 1, 2};
        auto indexBuffer = iGe::IndexBuffer::Create(indices.data(), indices.size());
        m_EmptyVertexArray->SetIndexBuffer(indexBuffer);
    }

    // Load model
    {
        // Bake
        // auto oriMesh = MeshBaker::LoadObjFile("assets/models/Icosphere.obj");
        // auto bakedMesh = MeshBaker::LoadObjFile("assets/models/" + oriMesh.Name + "_baked.obj");
        // MeshBaker::BakeTest(bakedMesh, oriMesh, 1024);

        m_OriginModel = MeshBaker::LoadObjFile("assets/models/Bayon Lion.obj");
        {
            auto vertices = m_OriginModel.Vertices;
            auto indices = m_OriginModel.Indices;
            m_OriginModelVertexArray = iGe::VertexArray::Create();

            auto vertexBuffer = iGe::VertexBuffer::Create(reinterpret_cast<float*>(vertices.data()),
                                                          vertices.size() * sizeof(MeshBaker::Vertex));
            iGe::BufferLayout layout = {{iGe::ShaderDataType::Float3, "a_Position"},
                                        {iGe::ShaderDataType::Float3, "a_Normal"},
                                        {iGe::ShaderDataType::Float2, "a_TexCoord"}};
            vertexBuffer->SetLayout(layout);
            m_OriginModelVertexArray->AddVertexBuffer(vertexBuffer);

            auto indexBuffer = iGe::IndexBuffer::Create(indices.data(), indices.size());
            m_OriginModelVertexArray->SetIndexBuffer(indexBuffer);
        }

        m_Model = MeshBaker::LoadObjFile("assets/models/Bayon Lion_baked.obj");
        {
            // Model displace map
            {
                int w, h;
                std::vector<float> displaces;
                std::string name = m_Model.Name + "_disp.exr";
                MeshBaker::ReadExrFile("assets/textures/" + name, w, h, displaces);

                iGe::TextureSpecification displaceMapSpec;
                displaceMapSpec.Width = w;
                displaceMapSpec.Height = h;
                displaceMapSpec.Format = iGe::ImageFormat::R32F;
                displaceMapSpec.GenerateMips = false;

                m_ModelDisplaceMap = iGe::Texture2D::Create(displaceMapSpec);
                m_ModelDisplaceMap->SetData(displaces.data(), displaces.size() * sizeof(float));
                m_ModelDisplaceMap->Bind(3);
            }

            // Model normal map
            {
                int w, h;
                std::vector<glm::vec3> normals;
                std::string name = m_Model.Name + "_norm.exr";
                MeshBaker::ReadExrFile("assets/textures/" + name, w, h, normals);

                iGe::TextureSpecification normalMapSpec;
                normalMapSpec.Width = w;
                normalMapSpec.Height = h;
                normalMapSpec.Format = iGe::ImageFormat::RGB32F;
                normalMapSpec.GenerateMips = false;

                m_ModelNormalMap = iGe::Texture2D::Create(normalMapSpec);
                m_ModelNormalMap->SetData(normals.data(), normals.size() * sizeof(glm::vec3));
                m_ModelNormalMap->Bind(4);
            }

            // Vertices data
            {
                auto vertices = m_Model.Vertices;
                auto indices = m_Model.Indices;
                m_ModelVertexArray = iGe::VertexArray::Create();

                auto vertexBuffer = iGe::VertexBuffer::Create(reinterpret_cast<float*>(vertices.data()),
                                                              vertices.size() * sizeof(MeshBaker::Vertex));
                iGe::BufferLayout layout = {{iGe::ShaderDataType::Float3, "a_Position"},
                                            {iGe::ShaderDataType::Float3, "a_Normal"},
                                            {iGe::ShaderDataType::Float2, "a_TexCoord"}};
                vertexBuffer->SetLayout(layout);
                m_ModelVertexArray->AddVertexBuffer(vertexBuffer);

                auto indexBuffer = iGe::IndexBuffer::Create(indices.data(), indices.size());
                m_ModelVertexArray->SetIndexBuffer(indexBuffer);
            }

            // Software tessellation buffer
            {
                std::vector<glm::vec3> positions;
                std::vector<glm::vec3> normals;
                std::vector<glm::vec2> texcoords;
                for (const auto vertex: m_Model.Vertices) {
                    positions.push_back(vertex.Position);
                    normals.push_back(vertex.Normal);
                    texcoords.push_back(vertex.TexCoord);
                }

                m_ModelPositionBuffer = iGe::Buffer::Create(reinterpret_cast<void*>(positions.data()),
                                                            positions.size() * sizeof(glm::vec3));
                m_ModelNormalBuffer = iGe::Buffer::Create(reinterpret_cast<void*>(normals.data()),
                                                          normals.size() * sizeof(glm::vec3));
                m_ModelTexCoordBuffer = iGe::Buffer::Create(reinterpret_cast<void*>(texcoords.data()),
                                                            texcoords.size() * sizeof(glm::vec2));
                m_ModelQuadIndexBuffer = iGe::Buffer::Create(reinterpret_cast<void*>(m_Model.Indices.data()),
                                                             m_Model.Indices.size() * sizeof(std::uint32_t));
            }

            // NTF model
            m_NTFModel = NTF::Model::Load("assets/ntfs/Bayon Lion_baked.ntf");
            m_NTFBuffers.Create(m_NTFModel);

            // LUT data (for ablation study)
            m_LUTData = LUT::LoadFromCSV("assets/luts/Bayon Lion_baked_quads_lut.csv");
            m_LUTBuffers.Create(m_LUTData);

            // Build quad edge mapping for edge-based tessellation factors
            auto quadCount = m_Model.Indices.size() / 4;
            m_QuadEdgeMapping = m_Model.BuildQuadEdgeMapping();
            m_EdgeCount = m_QuadEdgeMapping.EdgeCount;

            // Create buffer for quad-to-edge ID mapping (uvec4 per quad)
            m_QuadEdgeIdBuffer = iGe::Buffer::Create(reinterpret_cast<void*>(m_QuadEdgeMapping.QuadEdgeIds.data()),
                                                     m_QuadEdgeMapping.QuadEdgeIds.size() * sizeof(glm::uvec4));

            // Create buffer for per-edge accumulated tessellation factors (uint per edge)
            m_EdgeTessFactorBuffer = iGe::Buffer::Create(nullptr, m_EdgeCount * sizeof(std::uint32_t));
            m_InnerTessFactorBuffer = iGe::Buffer::Create(nullptr, quadCount * sizeof(glm::uvec2));
        }
    }

    // Software Tessellation
    {
        // RasterizerData
        m_TessellatorData = iGe::CreateScope<TessellatorData>();
        m_TessellatorDataUniform = iGe::Buffer::Create(nullptr, sizeof(TessellatorData));

        // Input buffer
        auto positions = m_Model.GetPositionArray();
        m_VertexBuffer =
                iGe::Buffer::Create(reinterpret_cast<void*>(positions.data()), positions.size() * sizeof(glm::vec3));
        m_VertexBuffer->Bind(10, iGe::BufferType::Storage);

        auto indices = m_Model.GetIndexArray();
        m_IndexBuffer = iGe::Buffer::Create(indices.data(), indices.size() * sizeof(std::uint32_t));
        m_IndexBuffer->Bind(11, iGe::BufferType::Storage);

        m_SubBufferIn = iGe::Buffer::Create(nullptr, indices.size() * std::pow(2, kMaxLodLevel) * sizeof(glm::uvec2));
        m_SubBufferIn->Bind(12, iGe::BufferType::Storage);

        //m_TessFactorBuffer = iGe::Buffer::Create(nullptr, triSize * sizeof(glm::uvec2));
        //m_TessFactorBuffer->Bind(12, iGe::BufferType::Storage);

        // Output buffer
        m_SubBufferCounter = iGe::Buffer::Create(nullptr, sizeof(glm::uvec2));
        m_SubBufferCounter->Bind(20, iGe::BufferType::Storage);

        m_SubBufferOut = iGe::Buffer::Create(nullptr, indices.size() * std::pow(2, kMaxLodLevel) * sizeof(glm::uvec2));
        m_SubBufferOut->Bind(21, iGe::BufferType::Storage);

        // Initial data
        int triCount = indices.size() / 3;

        std::vector<glm::uvec2> initialSubBuffer;
        for (int i = 0; i < triCount; ++i) { initialSubBuffer.push_back(glm::uvec2{1, i}); }
        m_SubBufferIn->SetData(reinterpret_cast<void*>(initialSubBuffer.data()),
                               initialSubBuffer.size() * sizeof(glm::uvec2));

        glm::uvec2 initialSubBufferCounter{triCount, triCount};
        m_SubBufferCounter->SetData(glm::gtc::value_ptr(initialSubBufferCounter), sizeof(glm::uvec2));
    }

    // SoftWare rasterization
    {
        auto& window = iGe::Application::Get().GetWindow();
        auto width = window.GetWidth();
        auto height = window.GetHeight();

        // Depth buffer
        iGe::TextureSpecification specification;
        specification.Width = width;
        specification.Height = height;
        specification.Format = iGe::ImageFormat::R32F;
        specification.GenerateMips = false;
        m_DepthBuffer = iGe::Texture2D::Create(specification);

        // Packed buffer
        m_Packed64Buffer = iGe::Buffer::Create(nullptr, width * height * sizeof(std::uint64_t));
    }

    // Set Model bbx
    m_ModelCenter = m_Model.Center;
    m_ModelRadius = m_Model.Radius;
    m_CameraPosition = m_ModelCenter + glm::vec3{0.0f, 0.0f, 2 * m_ModelRadius};

    // Create camera data uniform
    m_PerFrameData = iGe::CreateScope<PerFrameData>();
    m_PerFrameDataUniform = iGe::Buffer::Create(nullptr, sizeof(PerFrameData));

    m_GraphicsShaderLibrary.Load("Lighting", "assets/shaders/glsl/Lighting.json");
    m_GraphicsShaderLibrary.Load("FullScreen", "assets/shaders/glsl/FullScreen.json");
    m_GraphicsShaderLibrary.Load("HWTessellator", "assets/shaders/glsl/HWTessellator.json");

    // m_ComputeShaderLibrary.Load("CalTessFactor", "assets/shaders/glsl/CalTessFactor.json");
    // m_ComputeShaderLibrary.Load("SWTessellator", "assets/shaders/glsl/SWTessellator.json");
    // m_ComputeShaderLibrary.Load("ClearDepth", "assets/shaders/glsl/ClearDepth.json");
    // m_ComputeShaderLibrary.Load("SWRasterizer", "assets/shaders/glsl/SWRasterizer.json");

    m_GraphicsShaderLibrary.Load("NormalLighting", "assets/shaders/other/NormalLighting.json");
    m_ComputeShaderLibrary.Load("TFCalculator", "assets/shaders/other/TFCalculator.json");
    m_ComputeShaderLibrary.Load("TFCalculatorLUT", "assets/shaders/other/TFCalculatorLUT.json");
    m_MeshShaderLibrary.Load("SWTessellator", "assets/shaders/other/SWTessellator.json");

    glGenQueries(2, m_QueryIDs.data());
}

void RuntimeLodLayer::OnUpdate(iGe::Timestep ts) {
    // Camera movement
    if (!ImGui::GetIO().WantCaptureMouse) {
        if (iGe::Input::IsMouseButtonPressed(iGeKey::MouseLeft)) {
            ModelRotation();
        } else if (iGe::Input::IsMouseButtonPressed(iGeKey::MouseMiddle)) {
            ViewTranslation();
        }
    }

    // iGe::RenderCommand::SetClearColor(glm::vec4{225.0f / 255.0f, 245.0f / 255.0f, 220.0f / 255.0f, 1.0f});
    iGe::RenderCommand::SetClearColor(glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});
    iGe::RenderCommand::Clear();

    m_Camera.SetPosition(m_CameraPosition);
    m_Camera.SetRotation(m_CameraRotation);

    // static auto startTime = std::chrono::high_resolution_clock::now();
    // auto currentTime = std::chrono::high_resolution_clock::now();
    // float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    // m_ModelTransform = glm::gtc::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // Update perframe data
    m_PerFrameData->ViewPos = m_Camera.GetPosition();
    m_PerFrameData->_padding_ViewPos = 0;
    m_PerFrameData->NormalMatrix = glm::mat4{glm::transpose(glm::inverse(glm::mat3{m_ModelTransform}))};
    m_PerFrameDataUniform->SetData(m_PerFrameData.get(), sizeof(PerFrameData));
    m_PerFrameDataUniform->Bind(1, iGe::BufferType::Uniform);

    iGe::Renderer::BeginScene(m_Camera);
    {
        const float width = m_DepthBuffer->GetWidth();
        const float height = m_DepthBuffer->GetHeight();
        std::uint32_t quadSize = m_Model.GetIndexArray().size() / 4;

        //// Use compute shader to tessellation
        //{
        //    m_TessellatorData->ScreenSize = glm::uvec2{width, height};
        //    m_TessellatorData->TriSize = triSize;
        //    m_TessellatorData->DisplaceMapScale = m_DisplaceMapScale;
        //    m_TessellatorDataUniform->SetData(m_TessellatorData.get(), sizeof(TessellatorData));
        //    m_TessellatorDataUniform->Bind(2, iGe::BufferType::Uniform);
        //
        //    m_VertexBuffer->Bind(10, iGe::BufferType::Storage);
        //    m_IndexBuffer->Bind(11, iGe::BufferType::Storage);
        //    m_SubBufferIn->Bind(12, iGe::BufferType::Storage);
        //    m_SubBufferCounter->Bind(20, iGe::BufferType::Storage);
        //    m_SubBufferOut->Bind(21, iGe::BufferType::Storage);
        //
        //    glm::uvec2 counter;
        //    m_SubBufferCounter->GetData(glm::gtc::value_ptr(counter), sizeof(glm::uvec2));
        //    m_SubBufferCounter->SetData(glm::gtc::value_ptr(glm::uvec2{0, counter.x}), sizeof(glm::uvec2));
        //
        //    glm::vec3 groupSize = glm::vec3{(counter.x + 31) / 32, 1, 1};
        //    iGe::Renderer::Dispatch(m_ComputeShaderLibrary.Get("SWTessellator"), groupSize, m_ModelTransform);
        //
        //    // Swap ping-pong buffer
        //    std::swap(m_SubBufferIn, m_SubBufferOut);
        //}

        //// Use compute shader to rasterization
        //{
        //    m_TessellatorDataUniform->Bind(2, iGe::BufferType::Uniform);
        //
        //    // Clear depth buffer
        //    m_DepthBuffer->BindImage(5);
        //    m_Packed64Buffer->Bind(6, iGe::BufferType::Storage);
        //
        //    glm::vec3 groupSize = glm::vec3{(width + 7) / 8, (height + 7) / 8, 1};
        //    iGe::Renderer::Dispatch(m_ComputeShaderLibrary.Get("ClearDepth"), groupSize, m_ModelTransform);
        //
        //    // Software rasterization
        //    m_VertexBuffer->Bind(10, iGe::BufferType::Storage);
        //    m_IndexBuffer->Bind(11, iGe::BufferType::Storage);
        //
        //    groupSize = glm::vec3{(triSize + 31) / 32, 1, 1};
        //    iGe::Renderer::Dispatch(m_ComputeShaderLibrary.Get("SWRasterizer"), groupSize, m_ModelTransform);
        //}

        // Draw model
        {
            if (m_TessellationMode == 0) {
                iGe::Renderer::SubmitTris(m_GraphicsShaderLibrary.Get("NormalLighting"), m_OriginModelVertexArray,
                                          m_ModelTransform);
            } else {
                m_MaxDist = m_ModelRadius * 4.0f;
                m_TessellatorData->ScreenSize = glm::uvec2{(std::uint32_t) width, (std::uint32_t) height};
                m_TessellatorData->TessellationMode = m_TessellationMode;
                m_TessellatorData->TargetTessFactor = m_TargetTessFactor;
                m_TessellatorData->LineOption = m_LineOption ? 1 : 0;
                m_TessellatorData->EpsilonOption = m_EpsilonOption ? 1 : 0;
                m_TessellatorData->QuadSize = quadSize;
                m_TessellatorData->MinDist = m_MinDist;
                m_TessellatorData->MaxDist = m_MaxDist;
                m_TessellatorData->TargetPixel = m_TargetPixel;
                m_TessellatorData->MaxCurvature = m_MaxCurvature;
                m_TessellatorData->EpsilonCoefficient = m_EpsilonCoefficient;
                if (!m_LockCameraPosition) {
                    m_TessellatorData->ViewPos = m_Camera.GetPosition();
                    m_TessellatorData->Model = m_ModelTransform;
                    m_TessellatorData->View = m_Camera.GetViewMatrix();
                    m_TessellatorData->Projection = m_Camera.GetProjectionMatrix();
                }
                m_TessellatorDataUniform->SetData(m_TessellatorData.get(), sizeof(TessellatorData));
                m_TessellatorDataUniform->Bind(2, iGe::BufferType::Uniform);
                m_ModelDisplaceMap->Bind(3);
                m_ModelNormalMap->Bind(4);

                // // Hardware tessellation and rendering
                // iGe::Renderer::SubmitPatches(m_GraphicsShaderLibrary.Get("HWTessellator"), m_ModelVertexArray, 4,
                //                              m_ModelTransform);

                m_ModelPositionBuffer->Bind(5, iGe::BufferType::Storage);
                m_ModelNormalBuffer->Bind(6, iGe::BufferType::Storage);
                m_ModelTexCoordBuffer->Bind(7, iGe::BufferType::Storage);
                m_ModelQuadIndexBuffer->Bind(8, iGe::BufferType::Storage);

                m_QuadEdgeIdBuffer->Bind(9, iGe::BufferType::Storage);
                m_EdgeTessFactorBuffer->Bind(10, iGe::BufferType::Storage);
                m_InnerTessFactorBuffer->Bind(11, iGe::BufferType::Storage);

                std::vector<std::uint32_t> zeroData(m_EdgeCount, 0);
                m_EdgeTessFactorBuffer->SetData(zeroData.data(), m_EdgeCount * sizeof(std::uint32_t));

                if (m_TessellationMode == 4) {
                    // NTF (Neural Tessellation Factor) mode
                    m_NTFBuffers.Bind(12, 13, 14);

                    static double totalTime = 0.0;
                    static int loop = 0;
                    if (!m_ReStartCountTime) {
                        totalTime = 0.0;
                        loop = 0;
                    }

                    GLuint64 timeElapsed = 0;
                    glBeginQuery(GL_TIME_ELAPSED, m_QueryIDs[0]);
                    iGe::Renderer::Dispatch(m_ComputeShaderLibrary.Get("TFCalculator"),
                                            glm::vec3{(quadSize + 31) / 32, 1, 1}, m_ModelTransform);
                    glEndQuery(GL_TIME_ELAPSED);
                    glGetQueryObjectui64v(m_QueryIDs[0], GL_QUERY_RESULT, &timeElapsed);
                    double time = timeElapsed / 1000000.0;

                    if (m_ReStartCountTime) {
                        totalTime += time;
                        loop += 1;
                    }

                    std::cout << std::format("Mode {}: Neural Network GPU Time: {} ms", m_TessellationMode,
                                             totalTime / loop)
                              << std::endl;
                } else if (m_TessellationMode == 5) {
                    // LUT (Lookup Table) mode - for ablation study
                    m_LUTBuffers.Bind(15, 16);

                    static double totalTimeLUT = 0.0;
                    static int loopLUT = 0;
                    if (!m_ReStartCountTime) {
                        totalTimeLUT = 0.0;
                        loopLUT = 0;
                    }

                    GLuint64 timeElapsed = 0;
                    glBeginQuery(GL_TIME_ELAPSED, m_QueryIDs[0]);
                    iGe::Renderer::Dispatch(m_ComputeShaderLibrary.Get("TFCalculatorLUT"),
                                            glm::vec3{(quadSize + 31) / 32, 1, 1}, m_ModelTransform);
                    glEndQuery(GL_TIME_ELAPSED);
                    glGetQueryObjectui64v(m_QueryIDs[0], GL_QUERY_RESULT, &timeElapsed);
                    double time = timeElapsed / 1000000.0;

                    if (m_ReStartCountTime) {
                        totalTimeLUT += time;
                        loopLUT += 1;
                    }

                    std::cout << std::format("Mode {}: LUT Lookup GPU Time: {} ms", m_TessellationMode,
                                             totalTimeLUT / loopLUT)
                              << std::endl;
                }

                // Software Tessellation
                static double totalTime = 0.0;
                static int loop = 0;
                if (!m_ReStartCountTime) {
                    totalTime = 0.0;
                    loop = 0;
                }

                GLuint64 timeElapsed = 0;
                glBeginQuery(GL_TIME_ELAPSED, m_QueryIDs[0]);
                {
                    iGe::Renderer::DispatchTask(m_MeshShaderLibrary.Get("SWTessellator"), 0, quadSize,
                                                m_ModelTransform);
                }
                glEndQuery(GL_TIME_ELAPSED);
                glGetQueryObjectui64v(m_QueryIDs[0], GL_QUERY_RESULT, &timeElapsed);
                double time = timeElapsed / 1000000.0;

                if (m_ReStartCountTime) {
                    totalTime += time;
                    loop += 1;
                }

                std::cout << std::format("Mode {}: Tessellation GPU Time: {} ms", m_TessellationMode, totalTime / loop)
                          << std::endl;

                // Triangle count
                std::vector<std::uint32_t> edgeBuffer(m_EdgeCount);
                m_EdgeTessFactorBuffer->GetData(edgeBuffer.data(), m_EdgeCount * sizeof(std::uint32_t));
                std::vector<glm::uvec2> innerBuffer(quadSize);
                m_InnerTessFactorBuffer->GetData(innerBuffer.data(), quadSize * sizeof(glm::uvec2));
                std::uint32_t totalTriCount = 0;
                for (int i = 0; i < quadSize; ++i) {
                    auto edgeIds = m_QuadEdgeMapping.QuadEdgeIds[i];
                    auto eBottom = static_cast<std::uint32_t>(std::round(edgeBuffer[edgeIds.x] / 2.0f));
                    auto eRight = static_cast<std::uint32_t>(std::round(edgeBuffer[edgeIds.y] / 2.0f));
                    auto eTop = static_cast<std::uint32_t>(std::round(edgeBuffer[edgeIds.z] / 2.0f));
                    auto eLeft = static_cast<std::uint32_t>(std::round(edgeBuffer[edgeIds.w] / 2.0f));
                    auto innerU = innerBuffer[i].x;
                    auto innerV = innerBuffer[i].y;
                    totalTriCount += innerU * innerV * 2;
                    totalTriCount += (innerU + innerV) * 2;
                    totalTriCount += eBottom + eRight + eTop + eLeft;
                    // std::cout << std::format(
                    //                      "Quad {}: Edge Tess Factors: ({}, {}, {}, {}), Inner Tess Factors: ({}, "
                    //                      "{}), Tri Count: {}",
                    //                      i, eBottom, eRight, eTop, eLeft, innerU, innerV,
                    //                      innerU * innerV * 2 + (innerU + innerV) * 2 + eBottom + eRight + eTop +
                    //                              eLeft)
                    //           << std::endl;
                }
                std::cout << std::format("Total Triangle Count: {}", totalTriCount) << std::endl;
            }
        }

        //m_TessellatorDataUniform->Bind(2, iGe::BufferType::Uniform);
        //m_DepthBuffer->BindImage(5);
        //m_Packed64Buffer->Bind(6, iGe::BufferType::Storage);
        //m_VertexBuffer->Bind(10, iGe::BufferType::Storage);
        //m_IndexBuffer->Bind(11, iGe::BufferType::Storage);
        //iGe::Renderer::Submit(m_GraphicsShaderLibrary.Get("FullScreen"), m_EmptyVertexArray, m_ModelTransform);
    }
    iGe::Renderer::EndScene();
}

void RuntimeLodLayer::OnImGuiRender() {
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
    ImGui::Begin("Settings");
    {
        if (ImGui::BeginTable("SettingsTable", 2, ImGuiTableFlags_SizingStretchProp)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Target Tess Factor");
            ImGui::TableSetColumnIndex(1);
            ImGui::SliderInt("##TessFactor", reinterpret_cast<int*>(&m_TargetTessFactor), 1, 64);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Display Line");
            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##DisplayLine", &m_LineOption);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Display Epsilon");
            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##DisplayEpsilon", &m_EpsilonOption);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Tessellation Mode");
            ImGui::TableSetColumnIndex(1);
            static const char* options[] = {"Original",
                                            "Distance-Based Tessellation",
                                            "Screen-Space Tessellation",
                                            "Normal-Based Tessellation",
                                            "NTF Tessellation",
                                            "LUT Tessellation (Ablation)"};
            ImGui::Combo("##TessellationMode", &m_TessellationMode, options, IM_ARRAYSIZE(options));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Min Distance");
            ImGui::TableSetColumnIndex(1);
            ImGui::InputFloat("##MinDistance", reinterpret_cast<float*>(&m_MinDist));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Max Distance");
            ImGui::TableSetColumnIndex(1);
            ImGui::InputFloat("##MaxDistance", reinterpret_cast<float*>(&m_MaxDist));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Target Pixel");
            ImGui::TableSetColumnIndex(1);
            ImGui::InputFloat("##TargetPixel", reinterpret_cast<float*>(&m_TargetPixel));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Max Curvature");
            ImGui::TableSetColumnIndex(1);
            ImGui::InputFloat("##MaxCurvature", reinterpret_cast<float*>(&m_MaxCurvature));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Epsilon Coefficient");
            ImGui::TableSetColumnIndex(1);
            ImGui::InputFloat("##EpsilonCoefficient", reinterpret_cast<float*>(&m_EpsilonCoefficient));

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Lock Camera Position");
            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##LockCameraPosition", &m_LockCameraPosition);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Restart Count Time");
            ImGui::TableSetColumnIndex(1);
            ImGui::Checkbox("##ReStartCountTime", &m_ReStartCountTime);

            ImGui::EndTable();
        }
    }
    ImGui::End();

    //static bool show = true;
    //ImGui::ShowDemoWindow(&show);
}

void RuntimeLodLayer::OnEvent(iGe::Event& event) {
    iGe::EventDispatcher dispatcher(event);

    dispatcher.Dispatch<iGe::WindowResizeEvent>(
            std::bind(&RuntimeLodLayer::OnWindowResizeEvent, this, std::placeholders::_1));

    dispatcher.Dispatch<iGe::KeyPressedEvent>(
            std::bind(&RuntimeLodLayer::OnKeyPressedEvent, this, std::placeholders::_1));

    dispatcher.Dispatch<iGe::MouseScrolledEvent>(
            std::bind(&RuntimeLodLayer::OnMouseScrolledEvent, this, std::placeholders::_1));

    dispatcher.Dispatch<iGe::MouseButtonPressedEvent>(
            std::bind(&RuntimeLodLayer::OnMouseButtonPresseddEvent, this, std::placeholders::_1));

    dispatcher.Dispatch<iGe::MouseButtonReleasedEvent>(
            std::bind(&RuntimeLodLayer::OnMouseButtonReleasedEvent, this, std::placeholders::_1));
}

bool RuntimeLodLayer::OnWindowResizeEvent(iGe::WindowResizeEvent& event) {
    auto& window = iGe::Application::Get().GetWindow();

    // Resize camera
    float aspectRatio = float(window.GetWidth()) / float(window.GetHeight());
    m_Camera.SetProjection(60.0f, aspectRatio, 0.01f, 1000.0f);

    // Resize viewport
    iGe::Renderer::OnWindowResize(window.GetWidth(), window.GetHeight());

    // Resize software rasterizer texture
    iGe::TextureSpecification specification;
    specification.Width = window.GetWidth();
    specification.Height = window.GetHeight();
    specification.Format = iGe::ImageFormat::R32F;
    specification.GenerateMips = false;
    m_DepthBuffer = iGe::Texture2D::Create(specification);

    return false;
}

bool RuntimeLodLayer::OnMouseScrolledEvent(iGe::MouseScrolledEvent& event) {
    if (ImGui::GetIO().WantCaptureMouse) { return false; }

    auto speed = length(m_ModelCenter - m_CameraPosition);
    m_CameraPosition.z -= event.GetYOffset() * speed / 10.0f;
    return false;
}

bool RuntimeLodLayer::OnMouseButtonPresseddEvent(iGe::MouseButtonPressedEvent& event) {
    if (ImGui::GetIO().WantCaptureMouse) { return false; }

    m_LastMousePosition = glm::vec2{iGe::Input::GetMouseX(), iGe::Input::GetMouseY()};
    return false;
}

bool RuntimeLodLayer::OnMouseButtonReleasedEvent(iGe::MouseButtonReleasedEvent& event) {
    if (ImGui::GetIO().WantCaptureMouse) { return false; }

    m_LastMousePosition = glm::vec2{0.0f};
    return false;
}

void RuntimeLodLayer::ModelRotation() {
    auto& window = iGe::Application::Get().GetWindow();
    auto width = window.GetWidth();
    auto height = window.GetHeight();

    const double trackballradius = 0.6;
    const double rsqr = trackballradius * trackballradius;

    glm::vec3 oldPoint3D = glm::vec3{0.0f};
    {
        // calculate old hit sphere point3D
        double oldX = (2.0 * m_LastMousePosition.x - width) / width;
        double oldY = -(2.0 * m_LastMousePosition.y - height) / height;
        double old_x2y2 = oldX * oldX + oldY * oldY;

        oldPoint3D.x = oldX;
        oldPoint3D.y = oldY;
        if (old_x2y2 < 0.5 * rsqr) {
            oldPoint3D.z = std::sqrt(rsqr - old_x2y2);
        } else {
            oldPoint3D.z = 0.5 * rsqr / std::sqrt(old_x2y2);
        }
    }

    glm::vec3 newPoint3D = glm::vec3{0.0f};
    {
        glm::vec2 currentMousePos = glm::vec2{iGe::Input::GetMouseX(), iGe::Input::GetMouseY()};
        m_LastMousePosition = currentMousePos;

        // calculate new hit sphere point3D
        double newX = (2.0 * currentMousePos.x - width) / width;
        double newY = -(2.0 * currentMousePos.y - height) / height;
        double new_x2y2 = newX * newX + newY * newY;

        newPoint3D.x = newX;
        newPoint3D.y = newY;
        if (new_x2y2 < 0.5 * rsqr) {
            newPoint3D.z = std::sqrt(rsqr - new_x2y2);
        } else {
            newPoint3D.z = 0.5 * rsqr / std::sqrt(new_x2y2);
        }
    }

    glm::vec3 axis = glm::cross(oldPoint3D, newPoint3D); // corss product
    if (glm::length(axis) < 1e-7) {
        axis = glm::vec3(1.0f, 0.0f, 0.0f);
    } else {
        glm::normalize(axis);
    }

    // find the amount of rotation
    glm::vec3 d = oldPoint3D - newPoint3D;
    double t = 0.5 * glm::length(d) / trackballradius;
    if (t < -1.0) {
        t = -1.0;
    } else if (t > 1.0) {
        t = 1.0;
    }

    constexpr double PI = 3.14159265358979323846;
    double phi = 2.0 * std::asin(t);
    double angle = phi * 180.0 / PI;

    glm::mat4 translateToOrigin = glm::gtc::translate(glm::mat4{1.0f}, -m_ModelCenter);
    glm::mat4 translateBack = glm::gtc::translate(glm::mat4{1.0f}, m_ModelCenter);
    glm::mat4 rotate = glm::gtc::rotate(glm::mat4{1.0f}, static_cast<float>(glm::radians(angle)), axis);

    glm::mat4 rotateSelf = translateBack * rotate * translateToOrigin;
    m_ModelTransform = rotateSelf * m_ModelTransform;
}

void RuntimeLodLayer::ViewTranslation() {
    // Get mouse movement
    glm::vec2 mousePos = glm::vec2{iGe::Input::GetMouseX(), iGe::Input::GetMouseY()};
    glm::vec2 mouseDelta = mousePos - m_LastMousePosition;
    if (mouseDelta.x == 0 && mouseDelta.y == 0) { return; }
    m_LastMousePosition = mousePos;

    auto& window = iGe::Application::Get().GetWindow();
    const float windowWidth = static_cast<float>(window.GetWidth());
    const float windowHeight = static_cast<float>(window.GetHeight());

    const glm::vec3 modelCenter = m_ModelCenter;
    const glm::mat4 viewProj = m_Camera.GetViewProjectionMatrix();
    const glm::mat4 modelMatrix = m_ModelTransform;
    const glm::mat4 mvp = viewProj * modelMatrix;

    if (glm::abs(glm::determinant(mvp)) < 1e-6f) { return; }

    // Get NDC coordinate of the model center
    glm::vec4 clip = mvp * glm::vec4(modelCenter, 1.0f);
    glm::vec3 ndcCenter = glm::vec3(clip) / clip.w;

    // Compute offset in NDC space
    glm::vec3 ndcOffset = ndcCenter + glm::vec3{mouseDelta.x / windowWidth * 2.0f,
                                                -mouseDelta.y / windowHeight * 2.0f, // Y axis is flipped
                                                0.0f};

    // Convert NDC to world space
    glm::mat4 invMVP = glm::inverse(mvp);
    glm::vec4 offsetWorld = invMVP * glm::vec4(ndcOffset, 1.0f);
    offsetWorld /= offsetWorld.w;

    // Compute translation vector from current model center to offset point
    glm::vec3 translation = modelCenter - glm::vec3{offsetWorld};
    m_CameraMoveSpeed = glm::length(glm::vec2{translation}) / glm::length(mouseDelta);
    m_CameraPosition += glm::vec3{-mouseDelta.x * m_CameraMoveSpeed, mouseDelta.y * m_CameraMoveSpeed, 0.0f};
}

bool RuntimeLodLayer::OnKeyPressedEvent(iGe::KeyPressedEvent& event) {
    // F12 - Screenshot
    if (event.GetKeyCode() == iGeKey::F12) {
        // Generate filename with timestamp using C++20 chrono
        auto now = std::chrono::system_clock::now();
        auto timePoint = std::chrono::floor<std::chrono::seconds>(now);

        std::string filename = std::format("screenshots/screenshot_{:%Y%m%d_%H%M%S}.png", timePoint);

        SaveScreenshot(filename);
        return true;
    }
    return false;
}

void RuntimeLodLayer::SaveScreenshot(const std::string& filename) {
    auto& window = iGe::Application::Get().GetWindow();
    int width = window.GetWidth();
    int height = window.GetHeight();

    // Create screenshots directory if it doesn't exist
    std::filesystem::path filepath(filename);
    if (filepath.has_parent_path()) { std::filesystem::create_directories(filepath.parent_path()); }

    // Allocate buffer for pixel data (RGBA)
    std::vector<unsigned char> pixels(width * height * 4);

    // Read pixels from framebuffer
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // Flip image vertically (OpenGL origin is bottom-left)
    std::vector<unsigned char> flippedPixels(width * height * 4);
    for (int y = 0; y < height; ++y) {
        std::memcpy(flippedPixels.data() + y * width * 4, pixels.data() + (height - 1 - y) * width * 4, width * 4);
    }

    // Save as PNG using stb_image_write
    int result = stbi_write_png(filename.c_str(), width, height, 4, flippedPixels.data(), width * 4);

    if (result) {
        IGE_INFO("Screenshot saved: {}", filename);
    } else {
        IGE_ERROR("Failed to save screenshot: {}", filename);
    }
}
