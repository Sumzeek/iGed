module;
#include "iGeMacro.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

module iGed.ExampleLayer;

import iGe;

/////////////////////////////////////////////////////////////////////////////
// ExampleLayer ////////////////////////////////;/////////////////////////////
/////////////////////////////////////////////////////////////////////////////
ExampleLayer::ExampleLayer() : Layer{"Example"}, m_Camera{-1.6f, 1.6f, -0.9f, 0.9f}, m_CameraPosition{0.0f} {
    // Triangle
    {
        m_VertexArray = iGe::VertexArray::Create();

        float vertices[3 * 3 * 2] = {-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, -0.5f, 0.0f,
                                     0.0f,  1.0f,  0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f,  1.0f};
        auto vertexBuffer = iGe::VertexBuffer::Create(vertices, sizeof(vertices));
        iGe::BufferLayout layout = {{iGe::ShaderDataType::Float3, "a_Position"},
                                    {iGe::ShaderDataType::Float3, "a_Color"}};
        vertexBuffer->SetLayout(layout);
        m_VertexArray->AddVertexBuffer(vertexBuffer);

        uint32_t indices[3] = {0, 1, 2};
        auto indexBuffer = iGe::IndexBuffer::Create(indices, 3);
        m_VertexArray->SetIndexBuffer(indexBuffer);

        m_ShaderLibrary.Load("Color", "assets/shaders/glsl/Color.json");
    }

    // Square
    {
        m_SquareVertexArray = iGe::VertexArray::Create();

        float vertices[4 * 5] = {
                -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f,
                0.5f,  0.5f,  0.0f, 1.0f, 1.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
        };
        auto vertexBuffer = iGe::VertexBuffer::Create(vertices, sizeof(vertices));
        iGe::BufferLayout layout = {{iGe::ShaderDataType::Float3, "a_Position"},
                                    {iGe::ShaderDataType::Float2, "a_TexCoord"}};
        vertexBuffer->SetLayout(layout);
        m_SquareVertexArray->AddVertexBuffer(vertexBuffer);

        uint32_t indices[6] = {0, 1, 2, 2, 3, 0};
        auto indexBuffer = iGe::IndexBuffer::Create(indices, 6);
        m_SquareVertexArray->SetIndexBuffer(indexBuffer);

        auto shader = m_ShaderLibrary.Load("Texture", "assets/shaders/glsl/Texture.json");
        shader->Bind();

        m_Texture = iGe::Texture2D::Create("assets/textures/Checkerboard.png");
        m_iGameLogoTexture = iGe::Texture2D::Create("assets/textures/iGameLogo.png");
    }
}

void ExampleLayer::OnUpdate(iGe::Timestep ts) {
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
    }

    iGe::RenderCommand::SetClearColor(glm::vec4{0.5f, 0.5f, 0.5f, 1.0f});
    iGe::RenderCommand::Clear();

    m_Camera.SetPosition(m_CameraPosition);
    m_Camera.SetRotation(m_CameraRotation);

    static auto startTime = std::chrono::high_resolution_clock::now();
    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
    glm::mat4 model = glm::gtc::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    iGe::Renderer::BeginScene(m_Camera);
    {
        // Triangle
        iGe::Renderer::Submit(m_ShaderLibrary.Get("Color"), m_VertexArray, model);

        // Square
        //m_Texture->Bind(1);
        //iGe::Renderer::Submit(m_ShaderLibrary.Get("Texture"), m_SquareVertexArray);

        // iGame Logo
        //m_iGameLogoTexture->Bind(1);
        //iGe::Renderer::Submit(m_ShaderLibrary.Get("Texture"), m_SquareVertexArray);
    }
    iGe::Renderer::EndScene();
}

void ExampleLayer::OnImGuiRender() {
    ImGui::Begin("Settings");
    ImGui::Text("Hello World");
    ImGui::End();

    //static bool show = true;
    //ImGui::ShowDemoWindow(&show);
}

void ExampleLayer::OnEvent(iGe::Event& event) {
    //iGe::EventDispatcher dispatcher(event);
    //dispatcher.Dispatch<iGe::KeyPressedEvent>(std::bind(&ExampleLayer::OnPressedEvent, this, std::placeholders::_1));
}

bool ExampleLayer::OnPressedEvent(iGe::KeyPressedEvent& event) {
    if (event.GetKeyCode() == iGeKey::W) { m_CameraPosition.y -= m_CameraMoveSpeed; }
    if (event.GetKeyCode() == iGeKey::A) { m_CameraPosition.x += m_CameraMoveSpeed; }
    if (event.GetKeyCode() == iGeKey::S) { m_CameraPosition.y += m_CameraMoveSpeed; }
    if (event.GetKeyCode() == iGeKey::D) { m_CameraPosition.x -= m_CameraMoveSpeed; }
    if (event.GetKeyCode() == iGeKey::Q) { m_CameraRotation -= m_CameraRotationSpeed; }
    if (event.GetKeyCode() == iGeKey::E) { m_CameraRotation += m_CameraRotationSpeed; }

    return false;
}
