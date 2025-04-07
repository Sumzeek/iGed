#include "ExampleLayer.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

/////////////////////////////////////////////////////////////////////////////
// ExampleLayer /////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
ExampleLayer::ExampleLayer() : Layer{"Example"}, m_Camera{-1.6f, 1.6f, -0.9f, 0.9f}, m_CameraPosition{0.0f} {
    m_VertexArray = iGe::VertexArray::Create();

    float vertices[3 * 3 * 2] = {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.0f,
                                 0.0f,  1.0f,  0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f,  1.0f};
    m_VertexBuffer = iGe::VertexBuffer::Create(vertices, sizeof(vertices));
    iGe::BufferLayout layout = {{iGe::ShaderDataType::Float3, "a_Position"}, {iGe::ShaderDataType::Float3, "a_Color"}};
    m_VertexBuffer->SetLayout(layout);
    m_VertexArray->AddVertexBuffer(m_VertexBuffer);

    unsigned int indices[3] = {0, 1, 2};
    m_IndexBuffer = iGe::IndexBuffer::Create(indices, 3);
    m_VertexArray->SetIndexBuffer(m_IndexBuffer);

    std::string vertexSrc = R"(
        #version 330 core

        layout(location = 0) in vec3 a_Position;
        layout(location = 1) in vec3 a_Color;

        uniform mat4 u_ViewProjection;
        uniform mat4 u_Transform;

        out vec3 v_Position;
        out vec3 v_Color;

        void main() {
            v_Position = a_Position;
            v_Color = a_Color;

            gl_Position = u_ViewProjection * u_Transform * vec4(a_Position, 1.0f);
        }
    )";
    std::string fragmentSrc = R"(
        #version 330 core

        layout(location = 0) out vec4 color;

        in vec3 v_Position;
        in vec3 v_Color;

        void main() {
            color = vec4(v_Color, 1.0f);
        }
    )";
    m_Shader = iGe::Shader::Create("first Shader", vertexSrc, fragmentSrc);
}

void ExampleLayer::OnUpdate() {
    iGe::RenderCommand::SetClearColor(glm::vec4{0.5f, 0.5f, 0.5f, 1.0f});
    iGe::RenderCommand::Clear();

    m_Camera.SetPosition(m_CameraPosition);
    m_Camera.SetRotation(m_CameraRotation);

    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    glm::mat4 model = glm::gtc::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    iGe::Renderer::BeginScene(m_Camera);
    iGe::Renderer::Submit(m_Shader, m_VertexArray, model);
    iGe::Renderer::EndScene();
}

void ExampleLayer::OnImGuiRender() {
    ImGui::Begin("Test");
    ImGui::Text("Hello World");
    ImGui::End();

    //static bool show = true;
    //ImGui::ShowDemoWindow(&show);
}

void ExampleLayer::OnEvent(iGe::Event& event) {
    iGe::EventDispatcher dispatcher(event);
    dispatcher.Dispatch<iGe::KeyPressedEvent>(std::bind(&ExampleLayer::OnPressedEvent, this, std::placeholders::_1));

    //if (event.GetEventType() == iGe::EventType::KeyPressed) {
    //    if (auto* keyEvent = dynamic_cast<iGe::KeyPressedEvent*>(&event)) {
    //        if (keyEvent->GetKeyCode() == iGeKey::Tab) { IGE_INFO("Tab key is pressed (event)!"); }
    //        IGE_INFO("{0}", keyEvent->GetKeyCode());
    //    }
    //}
}

bool ExampleLayer::OnPressedEvent(iGe::KeyPressedEvent& event) {
    if (event.GetKeyCode() == iGeKey::W) { m_CameraPosition.y -= m_CameraMoveSpeed; }
    if (event.GetKeyCode() == iGeKey::A) { m_CameraPosition.x += m_CameraMoveSpeed; }
    if (event.GetKeyCode() == iGeKey::S) { m_CameraPosition.y += m_CameraMoveSpeed; }
    if (event.GetKeyCode() == iGeKey::D) { m_CameraPosition.x -= m_CameraMoveSpeed; }
    if (event.GetKeyCode() == iGeKey::Q) { m_CameraRotation -= m_CameraRotationSpeed; }
    if (event.GetKeyCode() == iGeKey::E) { m_CameraRotation += m_CameraRotationSpeed; }

    std::cout << m_CameraPosition.x << " " << m_CameraPosition.y << std::endl;

    return false;
}

/////////////////////////////////////////////////////////////////////////////
// Sandbox //////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
Sandbox::Sandbox() { PushLayer(new ExampleLayer{}); }

Sandbox::~Sandbox() {}
