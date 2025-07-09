module;
#include "iGeMacro.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

module iGed.RuntimeLodLayer;
import MeshFitting;
import std;
import glm;

/////////////////////////////////////////////////////////////////////////////
// RuntimeLodLayer //////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
RuntimeLodLayer::RuntimeLodLayer()
    : Layer{"Example"}, m_Camera{45.0f, 1280.0f / 720.0f, 0.01f, 1000.f}, m_CameraPosition{0.0f} {
    // Load model
    {
        m_Model = MeshFitting::LoadObjFile("assets/models/monsterfrog_subd.obj");
        auto vertices = m_Model.Vertices;
        auto indices = m_Model.Indices;

        m_ModelVertexArray = iGe::VertexArray::Create();

        auto vertexBuffer = iGe::VertexBuffer::Create(reinterpret_cast<float*>(vertices.data()),
                                                      vertices.size() * sizeof(MeshFitting::Vertex));
        iGe::BufferLayout layout = {{iGe::ShaderDataType::Float3, "a_Position"},
                                    {iGe::ShaderDataType::Float3, "a_Normal"},
                                    {iGe::ShaderDataType::Float2, "a_TexCoord"},
                                    {iGe::ShaderDataType::Float3, "a_Tangent"},
                                    {iGe::ShaderDataType::Float3, "a_BiTangent"}};
        vertexBuffer->SetLayout(layout);
        m_ModelVertexArray->AddVertexBuffer(vertexBuffer);

        auto indexBuffer = iGe::IndexBuffer::Create(indices.data(), indices.size());
        m_ModelVertexArray->SetIndexBuffer(indexBuffer);

        //// Model lod
        //{
        //    int triSize = indices.size() / 3;
        //
        //    // depth buffer
        //    auto& window = iGe::Application::Get().GetWindow();
        //    iGe::TextureSpecification specification;
        //    specification.Width = window.GetWidth();
        //    specification.Height = window.GetHeight();
        //    specification.Format = iGe::ImageFormat::R32F;
        //    specification.GenerateMips = false;
        //    m_DepthBuffer = iGe::Texture2D::Create(specification);
        //
        //    // Input buffer
        //    m_VertexBuffer =
        //            iGe::Buffer::Create(reinterpret_cast<void*>(vertices.data()), vertices.size() * sizeof(glm::vec3));
        //    m_VertexBuffer->Bind(10, iGe::BufferType::Storage);
        //
        //    m_IndexBuffer = iGe::Buffer::Create(indices.data(), indices.size() * sizeof(uint32_t));
        //    m_IndexBuffer->Bind(11, iGe::BufferType::Storage);
        //
        //    m_TessFactorBuffer = iGe::Buffer::Create(nullptr, triSize * sizeof(glm::uvec2));
        //    m_TessFactorBuffer->Bind(12, iGe::BufferType::Storage);
        //
        //    m_CounterBuffer = iGe::Buffer::Create(nullptr, sizeof(uint32_t));
        //    m_CounterBuffer->Bind(13, iGe::BufferType::Storage);
        //}
    }

    // Model displace map
    //auto mesh1 = MeshFitting::LoadObjFile("assets/models/Bunny_Sim.obj");
    //auto mesh2 = MeshFitting::LoadObjFile("assets/models/Bunny.obj");
    //m_ModelDisplaceMap = MeshFitting::GenerateDisplacementMap(mesh1, mesh2, 512);
    m_ModelNormalMap = iGe::Texture2D::Create("assets/textures/monsterfrog-n.bmp");
    m_ModelDisplaceMap = iGe::Texture2D::Create("assets/textures/monsterfrog-d.bmp");

    // Set Model bbx
    m_ModelCenter = m_Model.Center;
    m_ModelRadius = m_Model.Radius;
    m_CameraPosition = m_ModelCenter + glm::vec3{0.0f, 0.0f, 3 * m_ModelRadius};

    // Create camera data uniform
    m_PerFrameData = iGe::CreateScope<PerFrameData>();
    m_PerFrameDataUniform = iGe::Buffer::Create(nullptr, sizeof(PerFrameData));
    m_TessData = iGe::CreateScope<TessData>();
    m_TessDataUniform = iGe::Buffer::Create(nullptr, sizeof(TessData));

    m_GraphicsShaderLibrary.Load("assets/shaders/glsl/Lighting.glsl");
    m_GraphicsShaderLibrary.Load("assets/shaders/glsl/Test.glsl");
    m_ComputeShaderLibrary.Load("assets/shaders/glsl/CalTessFactor.glsl");
    m_ComputeShaderLibrary.Load("assets/shaders/glsl/SWRasterizer.glsl");
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

    iGe::RenderCommand::SetClearColor(glm::vec4{0.5f, 0.5f, 0.5f, 1.0f});
    iGe::RenderCommand::Clear();

    m_Camera.SetPosition(m_CameraPosition);
    m_Camera.SetRotation(m_CameraRotation);

    auto& window = iGe::Application::Get().GetWindow();
    m_TessData->ScreenSize = glm::uvec2{window.GetWidth(), window.GetHeight()};
    m_TessData->TriSize = m_ModelVertexArray->GetIndexBuffer()->GetCount() / 3;
    m_TessDataUniform->SetData(m_TessData.get(), sizeof(TessData));

    m_PerFrameData->ViewPos = m_Camera.GetPosition();
    m_PerFrameData->DisplaceMapScale = m_DisplaceMapScale;

    iGe::Renderer::BeginScene(m_Camera);
    {
        iGe::Ref<iGe::GraphicsShader> gShader;
        iGe::Ref<iGe::ComputeShader> cShader;

        //Tessllation();
        //SoftwareRasterization();

        // Draw model
        {
            m_PerFrameData->NormalMatrix = glm::transpose(glm::inverse(m_ModelTransform));
            m_PerFrameDataUniform->SetData(m_PerFrameData.get(), sizeof(PerFrameData));
            m_PerFrameDataUniform->Bind(1, iGe::BufferType::Uniform);

            m_ModelNormalMap->Bind(2);
            m_ModelDisplaceMap->Bind(3);
            iGe::Renderer::Submit(m_GraphicsShaderLibrary.Get("Test"), m_ModelVertexArray, m_ModelTransform);
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

    //Tessllation();
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

    //Tessllation();
}

void RuntimeLodLayer::Tessllation() {
    // Calculate tessellation factor
    auto shader = m_ComputeShaderLibrary.Get("CalTessFactor");
    shader->Bind();

    m_TessDataUniform->Bind(1, iGe::BufferType::Uniform);

    m_VertexBuffer->Bind(10, iGe::BufferType::Storage);
    m_IndexBuffer->Bind(11, iGe::BufferType::Storage);
    m_TessFactorBuffer->Bind(12, iGe::BufferType::Storage);
    m_CounterBuffer->Bind(13, iGe::BufferType::Storage);

    // Reset counter
    uint32_t zero = 0;
    m_CounterBuffer->SetData(&zero, sizeof(uint32_t));

    uint32_t triSize = m_ModelVertexArray->GetIndexBuffer()->GetCount() / 3;
    shader->Dispatch(((triSize + 31) / 32), 1, 1);
}

void RuntimeLodLayer::SoftwareRasterization() {
    // Use compute shader to rasterization
    auto shader = m_ComputeShaderLibrary.Get("SWRasterizer");
    shader->Bind();

    m_DepthBuffer->BindImage(5);
    m_VertexBuffer->Bind(10, iGe::BufferType::Storage);
    m_IndexBuffer->Bind(11, iGe::BufferType::Storage);

    uint32_t triSize = m_ModelVertexArray->GetIndexBuffer()->GetCount() / 3;
    shader->Dispatch(((triSize + 31) / 32), 1, 1);
}
