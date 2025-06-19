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
        m_Armadillo = MeshFitting::LoadObjFile("assets/models/Armadillo.obj");
        auto vertices = m_Armadillo.Vertices;
        auto indices = m_Armadillo.Indices;

        m_VertexArray = iGe::VertexArray::Create();

        auto vertexBuffer = iGe::VertexBuffer::Create(reinterpret_cast<float*>(vertices.data()),
                                                      vertices.size() * sizeof(glm::vec3));
        iGe::BufferLayout layout = {{iGe::ShaderDataType::Float3, "a_Position"}};
        vertexBuffer->SetLayout(layout);
        m_VertexArray->AddVertexBuffer(vertexBuffer);

        auto indexBuffer = iGe::IndexBuffer::Create(reinterpret_cast<uint32_t*>(indices.data()), indices.size());
        m_VertexArray->SetIndexBuffer(indexBuffer);
    }

    auto mesh1 = MeshFitting::LoadObjFile("assets/models/Bunny_Sim.obj");
    auto mesh2 = MeshFitting::LoadObjFile("assets/models/Bunny.obj");
    m_Fitter = MeshFitting::Fit(mesh1, mesh2);

    // Bunny model
    {
        m_Bunny = MeshFitting::LoadObjFile("assets/models/Bunny_Sim.obj");
        auto vertices = m_Bunny.Vertices;
        auto indices = m_Bunny.Indices;

        m_BunnyVertexArray = iGe::VertexArray::Create();

        auto vertexBuffer = iGe::VertexBuffer::Create(reinterpret_cast<float*>(vertices.data()),
                                                      vertices.size() * sizeof(glm::vec3));
        iGe::BufferLayout layout = {{iGe::ShaderDataType::Float3, "a_Position"}};
        vertexBuffer->SetLayout(layout);
        m_BunnyVertexArray->AddVertexBuffer(vertexBuffer);

        auto indexBuffer = iGe::IndexBuffer::Create(indices.data(), indices.size());
        m_BunnyVertexArray->SetIndexBuffer(indexBuffer);

        // Cube lod
        {
            int triSize = indices.size() / 3;

            // Input buffer
            m_VertexBuffer =
                    iGe::Buffer::Create(reinterpret_cast<void*>(vertices.data()), vertices.size() * sizeof(glm::vec3));
            m_VertexBuffer->Bind(10, iGe::BufferType::Storage);

            m_IndexBuffer = iGe::Buffer::Create(indices.data(), indices.size() * sizeof(uint32_t));
            m_IndexBuffer->Bind(11, iGe::BufferType::Storage);

            m_TessFactorBuffer = iGe::Buffer::Create(nullptr, triSize * sizeof(glm::uvec2));
            m_TessFactorBuffer->Bind(12, iGe::BufferType::Storage);

            m_CounterBuffer = iGe::Buffer::Create(nullptr, sizeof(uint32_t));
            m_CounterBuffer->Bind(13, iGe::BufferType::Storage);
        }
    }

    // Set Model bbx
    //m_ModelCenter = (m_Armadillo.Center + m_Bunny.Center) / 2;
    //m_ModelRadius = (m_Armadillo.Radius - m_Bunny.Radius) / 2;
    m_ModelCenter = m_Bunny.Center;
    m_ModelRadius = m_Bunny.Radius;
    m_CameraPosition = m_ModelCenter + glm::vec3{0.0f, 0.0f, 3 * m_ModelRadius};

    // Create camera data uniform
    m_TessDataUniform = iGe::Buffer::Create(nullptr, sizeof(glm::uvec2) + sizeof(uint32_t) + +sizeof(uint32_t));
    m_TessDataUniform->Bind(1, iGe::BufferType::Uniform);

    m_GraphicsShaderLibrary.Load("assets/shaders/glsl/Lighting.glsl");
    m_GraphicsShaderLibrary.Load("assets/shaders/glsl/Test.glsl");
    m_ComputeShaderLibrary.Load("assets/shaders/glsl/CalTessFactor.glsl");
}

void RuntimeLodLayer::OnUpdate(iGe::Timestep ts) {
    // Camera movement
    {
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
    glm::uvec2 screenSize{window.GetWidth(), window.GetHeight()};
    uint32_t triSize = m_BunnyVertexArray->GetIndexBuffer()->GetCount() / 3;
    m_TessDataUniform->SetData(&screenSize, sizeof(glm::uvec2));
    m_TessDataUniform->SetData(&triSize, sizeof(uint32_t), sizeof(glm::uvec2));

    iGe::Renderer::BeginScene(m_Camera);
    {
        iGe::Ref<iGe::GraphicsShader> gShader;
        iGe::Ref<iGe::ComputeShader> cShader;

        // Models
        //gShader = m_GraphicsShaderLibrary.Get("Lighting");
        //iGe::Renderer::Submit(gShader, m_VertexArray, m_ModelTransform);

        Tessllation();

        // Cube
        std::vector<glm::vec3> cubeVertices;
        std::vector<uint32_t> cubeIndices;
        {
            // Get vertex/index buffer
            auto vertices = m_Bunny.Vertices;
            auto indices = m_Bunny.Indices;

            uint32_t triSize = m_BunnyVertexArray->GetIndexBuffer()->GetCount() / 3;
            std::vector<glm::uvec2> tessFactors(triSize);
            m_TessFactorBuffer->GetData(tessFactors.data(), tessFactors.size() * sizeof(glm::uvec2));
            for (int i = 0; i < triSize; ++i) {
                uint32_t triId = tessFactors[i].y;

                std::array<glm::vec3, 3> v;
                v[0] = m_Fitter->Apply(vertices[indices[triId * 3]]);
                v[1] = m_Fitter->Apply(vertices[indices[triId * 3 + 1]]);
                v[2] = m_Fitter->Apply(vertices[indices[triId * 3 + 2]]);

                uint32_t packed = tessFactors[i].x;
                std::int32_t r = (packed >> 16) & 0xFF;
                std::int32_t g = (packed >> 8) & 0xFF;
                std::int32_t b = (packed >> 0) & 0xFF;
                std::int32_t a = (packed >> 24) & 0xFF;

                glm::vec3 center = (v[0] + v[1] + v[2]) / 3.0f;
                for (int j = 0; j < r + 1; ++j) {
                    float t1 = float(j) / (r + 1);
                    float t2 = float(j + 1) / (r + 1);
                    glm::vec3 p1 = glm::mix(v[0], v[1], t1);
                    glm::vec3 p2 = glm::mix(v[0], v[1], t2);

                    cubeVertices.push_back(center);
                    cubeVertices.push_back(p1);
                    cubeVertices.push_back(p2);

                    cubeIndices.push_back(cubeIndices.size());
                    cubeIndices.push_back(cubeIndices.size());
                    cubeIndices.push_back(cubeIndices.size());
                }

                for (int j = 0; j < g + 1; ++j) {
                    float t1 = float(j) / (g + 1);
                    float t2 = float(j + 1) / (g + 1);
                    glm::vec3 p1 = glm::mix(v[1], v[2], t1);
                    glm::vec3 p2 = glm::mix(v[1], v[2], t2);

                    cubeVertices.push_back(center);
                    cubeVertices.push_back(p1);
                    cubeVertices.push_back(p2);

                    cubeIndices.push_back(cubeIndices.size());
                    cubeIndices.push_back(cubeIndices.size());
                    cubeIndices.push_back(cubeIndices.size());
                }

                for (int j = 0; j < b + 1; ++j) {
                    float t1 = float(j) / (b + 1);
                    float t2 = float(j + 1) / (b + 1);
                    glm::vec3 p1 = glm::mix(v[2], v[0], t1);
                    glm::vec3 p2 = glm::mix(v[2], v[0], t2);

                    cubeVertices.push_back(center);
                    cubeVertices.push_back(p1);
                    cubeVertices.push_back(p2);

                    cubeIndices.push_back(cubeIndices.size());
                    cubeIndices.push_back(cubeIndices.size());
                    cubeIndices.push_back(cubeIndices.size());
                }
            }
        }

        // Draw cube
        {
            auto vertexArray = iGe::VertexArray::Create();

            auto vertexBuffer = iGe::VertexBuffer::Create(reinterpret_cast<float*>(cubeVertices.data()),
                                                          cubeVertices.size() * sizeof(glm::vec3));
            iGe::BufferLayout layout = {{iGe::ShaderDataType::Float3, "a_Position"}};
            vertexBuffer->SetLayout(layout);
            vertexArray->AddVertexBuffer(vertexBuffer);

            auto indexBuffer = iGe::IndexBuffer::Create(cubeIndices.data(), cubeIndices.size());
            vertexArray->SetIndexBuffer(indexBuffer);

            gShader = m_GraphicsShaderLibrary.Get("Test");
            iGe::Renderer::Submit(gShader, vertexArray, m_ModelTransform);

            //gShader = m_GraphicsShaderLibrary.Get("Test");
            //iGe::Renderer::Submit(gShader, m_BunnyVertexArray, m_ModelTransform);
        }
    }
    iGe::Renderer::EndScene();
}

void RuntimeLodLayer::OnImGuiRender() {
    ImGui::Begin("Settings");
    ImGui::Text("Hello World");
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

    // resize camera
    float aspectRatio = float(window.GetWidth()) / float(window.GetHeight());
    m_Camera.SetProjection(45.0f, aspectRatio, 0.01f, 1000.0f);

    // resize viewport
    iGe::Renderer::OnWindowResize(window.GetWidth(), window.GetHeight());

    return false;
}

bool RuntimeLodLayer::OnMouseScrolledEvent(iGe::MouseScrolledEvent& event) {
    auto speed = length(m_ModelCenter - m_CameraPosition);
    m_CameraPosition.z -= event.GetYOffset() * speed / 10.0f;

    return false;
}

bool RuntimeLodLayer::OnMouseButtonPresseddEvent(iGe::MouseButtonPressedEvent& event) {
    m_LastMousePosition = glm::vec2{iGe::Input::GetMouseX(), iGe::Input::GetMouseY()};

    return false;
}

bool RuntimeLodLayer::OnMouseButtonReleasedEvent(iGe::MouseButtonReleasedEvent& event) {
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

    Tessllation();
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

    Tessllation();
}

void RuntimeLodLayer::Tessllation() {
    // Calculate Tessellation factor
    m_VertexBuffer->Bind(10, iGe::BufferType::Storage);
    m_IndexBuffer->Bind(11, iGe::BufferType::Storage);
    m_TessFactorBuffer->Bind(12, iGe::BufferType::Storage);
    m_CounterBuffer->Bind(13, iGe::BufferType::Storage);

    // Reset counter
    uint32_t zero = 0;
    m_CounterBuffer->SetData(&zero, sizeof(uint32_t));

    auto shader = m_ComputeShaderLibrary.Get("CalTessFactor");
    uint32_t triSize = m_BunnyVertexArray->GetIndexBuffer()->GetCount() / 3;
    shader->Dispatch(((triSize + 31) / 32), 1, 1);
}
