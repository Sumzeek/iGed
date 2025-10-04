module;
#include "iGeMacro.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

module iGed.RuntimeLodLayer;
import MeshBaker;
import std;
import glm;

/////////////////////////////////////////////////////////////////////////////
// RuntimeLodLayer //////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
RuntimeLodLayer::RuntimeLodLayer()
    : Layer{"RuntimeLod"}, m_Camera{45.0f, 1280.0f / 720.0f, 0.01f, 10000.f}, m_CameraPosition{0.0f} {
    // Create empty VAO
    {
        m_EmptyVertexArray = iGe::VertexArray::Create();
        std::vector<uint32_t> indices = {0, 1, 2};
        auto indexBuffer = iGe::IndexBuffer::Create(indices.data(), indices.size());
        m_EmptyVertexArray->SetIndexBuffer(indexBuffer);
    }

    // Load model
    {
        // Test
        {
            m_SimModel = MeshBaker::LoadObjFile("assets/models/Cube - Copy.obj");
            //m_SimModel = MeshBaker::LoadObjFile("assets/models/Bunny_Sim.obj");
            //m_SimModel = MeshBaker::LoadObjFile("assets/models/Dark_Finger_Reef_Crab_Sim.obj");
            {
                auto vertices = m_SimModel.Vertices;
                auto indices = m_SimModel.Indices;

                m_SimModelVertexArray = iGe::VertexArray::Create();

                auto vertexBuffer = iGe::VertexBuffer::Create(reinterpret_cast<float*>(vertices.data()),
                                                              vertices.size() * sizeof(MeshBaker::Vertex));
                iGe::BufferLayout layout = {{iGe::ShaderDataType::Float3, "a_Position"},
                                            {iGe::ShaderDataType::Float3, "a_Normal"},
                                            {iGe::ShaderDataType::Float2, "a_TexCoord"}};
                vertexBuffer->SetLayout(layout);
                m_SimModelVertexArray->AddVertexBuffer(vertexBuffer);

                auto indexBuffer = iGe::IndexBuffer::Create(indices.data(), indices.size());
                m_SimModelVertexArray->SetIndexBuffer(indexBuffer);
            }

            m_OriModel = MeshBaker::LoadObjFile("assets/models/Icosphere.obj");
            //m_OriModel = MeshBaker::LoadObjFile("assets/models/Bunny.obj");
            //m_OriModel = MeshBaker::LoadObjFile("assets/models/Dark_Finger_Reef_Crab.obj");
            {
                auto vertices = m_OriModel.Vertices;
                auto indices = m_OriModel.Indices;

                m_OriModelVertexArray = iGe::VertexArray::Create();

                auto vertexBuffer = iGe::VertexBuffer::Create(reinterpret_cast<float*>(vertices.data()),
                                                              vertices.size() * sizeof(MeshBaker::Vertex));
                iGe::BufferLayout layout = {{iGe::ShaderDataType::Float3, "a_Position"},
                                            {iGe::ShaderDataType::Float3, "a_Normal"},
                                            {iGe::ShaderDataType::Float2, "a_TexCoord"}};
                vertexBuffer->SetLayout(layout);
                m_OriModelVertexArray->AddVertexBuffer(vertexBuffer);

                auto indexBuffer = iGe::IndexBuffer::Create(indices.data(), indices.size());
                m_OriModelVertexArray->SetIndexBuffer(indexBuffer);
            }
        }

        m_Model = MeshBaker::LoadObjFile("assets/models/Dark_Finger_Reef_Crab_Sim.obj");
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
        m_IndexBuffer = iGe::Buffer::Create(indices.data(), indices.size() * sizeof(uint32_t));
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

    // Model displace map
    //MeshBaker::Bake(m_OriModel, 1000);
    MeshBaker::Bake(m_SimModel, m_OriModel, 1000);
    int w, h;
    std::string name = m_OriModel.Name + "_displacement.exr";
    std::vector<float> displace = MeshBaker::ReadExrFile("assets/textures/" + name, w, h);

    iGe::TextureSpecification displaceMapSpec;
    displaceMapSpec.Width = w;
    displaceMapSpec.Height = h;
    displaceMapSpec.Format = iGe::ImageFormat::R32F;
    displaceMapSpec.GenerateMips = false;

    m_ModelDisplaceMap = iGe::Texture2D::Create(displaceMapSpec);
    m_ModelDisplaceMap->SetData(displace.data(), displace.size() * sizeof(float));
    //m_ModelDisplaceMap->Bind(3);

    // Set Model bbx
    m_ModelCenter = m_OriModel.Center;
    m_ModelRadius = m_OriModel.Radius;
    m_CameraPosition = m_ModelCenter + glm::vec3{0.0f, 0.0f, 3 * m_ModelRadius};

    // Create camera data uniform
    m_PerFrameData = iGe::CreateScope<PerFrameData>();
    m_PerFrameDataUniform = iGe::Buffer::Create(nullptr, sizeof(PerFrameData));

    m_GraphicsShaderLibrary.Load("Lighting", "assets/shaders/glsl/Lighting.json");
    m_GraphicsShaderLibrary.Load("FullScreen", "assets/shaders/glsl/FullScreen.json");
    m_GraphicsShaderLibrary.Load("HWTessellator", "assets/shaders/glsl/HWTessellator.json");

    m_ComputeShaderLibrary.Load("CalTessFactor", "assets/shaders/glsl/CalTessFactor.json");
    m_ComputeShaderLibrary.Load("SWTessellator", "assets/shaders/glsl/SWTessellator.json");
    m_ComputeShaderLibrary.Load("ClearDepth", "assets/shaders/glsl/ClearDepth.json");
    m_ComputeShaderLibrary.Load("SWRasterizer", "assets/shaders/glsl/SWRasterizer.json");
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

    iGe::RenderCommand::SetClearColor(glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
    iGe::RenderCommand::Clear();

    m_Camera.SetPosition(m_CameraPosition);
    m_Camera.SetRotation(m_CameraRotation);

    //m_ModelTransform = glm::gtc::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

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
        uint32_t triSize = m_Model.GetIndexArray().size() / 3;

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
            //iGe::Renderer::Submit(m_GraphicsShaderLibrary.Get("Lighting"), m_ModelVertexArray, m_ModelTransform);

            bool useDynamicTess = iGe::Input::IsMouseButtonPressed(iGeKey::MouseLeft) ||
                                  iGe::Input::IsMouseButtonPressed(iGeKey::MouseRight);
            if (!useDynamicTess) {
                iGe::Renderer::Submit(m_GraphicsShaderLibrary.Get("Lighting"), m_OriModelVertexArray, m_ModelTransform);
            } else {
                m_TessellatorData->ScreenSize = glm::uvec2{width, height};
                m_TessellatorData->TriSize = m_TargetTessFactor;
                m_TessellatorData->DisplaceMapScale = m_DisplaceMapScale;
                m_TessellatorDataUniform->SetData(m_TessellatorData.get(), sizeof(TessellatorData));
                m_TessellatorDataUniform->Bind(2, iGe::BufferType::Uniform);
                m_ModelDisplaceMap->Bind(3);
                iGe::Renderer::Submit(m_GraphicsShaderLibrary.Get("HWTessellator"), m_SimModelVertexArray,
                                      m_ModelTransform, true);
            }

            //m_TessellatorDataUniform->Bind(2, iGe::BufferType::Uniform);
            //m_DepthBuffer->BindImage(5);
            //m_Packed64Buffer->Bind(6, iGe::BufferType::Storage);
            //m_VertexBuffer->Bind(10, iGe::BufferType::Storage);
            //m_IndexBuffer->Bind(11, iGe::BufferType::Storage);
            //iGe::Renderer::Submit(m_GraphicsShaderLibrary.Get("FullScreen"), m_EmptyVertexArray, m_ModelTransform);
        }
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
            ImGui::Text("Displacement Scale");
            ImGui::TableSetColumnIndex(1);
            ImGui::SliderFloat("##DisplaceMapScale", &m_DisplaceMapScale, 0.0f, 10.0f);

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::Text("Target Tess Factor");
            ImGui::TableSetColumnIndex(1);
            ImGui::SliderInt("##TessFactor", reinterpret_cast<int*>(&m_TargetTessFactor), 1, 64);

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
    m_Camera.SetProjection(45.0f, aspectRatio, 0.01f, 1000.0f);

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
            newPoint3D.z = sqrt(rsqr - new_x2y2);
        } else {
            newPoint3D.z = 0.5 * rsqr / sqrt(new_x2y2);
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
