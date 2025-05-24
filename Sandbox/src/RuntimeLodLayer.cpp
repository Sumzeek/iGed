module;
#include "iGeMacro.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

module iGed.RuntimeLodLayer;
import glm;

/////////////////////////////////////////////////////////////////////////////
// RuntimeLodLayer //////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
RuntimeLodLayer::RuntimeLodLayer()
    : Layer{"Example"}, m_Camera{45.0f, 1280.0f / 720.0f, 0.01f, 1000.f}, m_CameraPosition{0.0f} {
    // Load model
    LoadModel("assets/models/Armadillo.obj");

    // Cube
    {
        m_CubeVertexArray = iGe::VertexArray::Create();

        std::vector<glm::vec3> vertices = {glm::vec3{-0.5f, -0.5f, 0.5f},  glm::vec3{0.5f, -0.5f, 0.5f},
                                           glm::vec3{0.5f, 0.5f, 0.5f},    glm::vec3{-0.5f, 0.5f, 0.5f},
                                           glm::vec3{-0.5f, -0.5f, -0.5f}, glm::vec3{0.5f, -0.5f, -0.5f},
                                           glm::vec3{0.5f, 0.5f, -0.5f},   glm::vec3{-0.5f, 0.5f, -0.5f}};
        std::vector<uint32_t> indices = {0, 1, 2, 2, 3, 0, 1, 5, 6, 6, 2, 1, 5, 4, 7, 7, 6, 5,
                                         4, 0, 3, 3, 7, 4, 3, 2, 6, 6, 7, 3, 4, 5, 1, 1, 0, 4};

        auto vertexBuffer = iGe::VertexBuffer::Create(reinterpret_cast<float*>(vertices.data()),
                                                      vertices.size() * sizeof(glm::vec3));
        iGe::BufferLayout layout = {{iGe::ShaderDataType::Float3, "a_Position"}};
        vertexBuffer->SetLayout(layout);
        m_CubeVertexArray->AddVertexBuffer(vertexBuffer);

        auto indexBuffer = iGe::IndexBuffer::Create(indices.data(), indices.size() * sizeof(uint32_t));
        m_CubeVertexArray->SetIndexBuffer(indexBuffer);

        m_ModelCenter = glm::vec3{0.0f, 0.0f, 0.0f};
        m_CameraPosition = glm::vec3{0.0f, 0.0f, 3.0f};
    }

    // Create camera data uniform
    m_CameraDataUniform = iGe::Buffer::Create(nullptr, sizeof(glm::vec3));
    m_CameraDataUniform->Bind(1, iGe::BufferType::Uniform);

    m_GraphicsShaderLibrary.Load("assets/shaders/glsl/Lighting.glsl");
    m_GraphicsShaderLibrary.Load("assets/shaders/glsl/Test.glsl");
    m_GraphicsShaderLibrary.Load("assets/shaders/glsl/Tessellation.glsl");
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
    m_CameraDataUniform->SetData(&m_CameraPosition, sizeof(m_CameraPosition));

    iGe::Renderer::BeginScene(m_Camera);
    {
        // Models
        //auto shader = m_GraphicsShaderLibrary.Get("Lighting");
        //iGe::Renderer::Submit(shader, m_VertexArray, m_ModelTransform);

        // Cube
        auto shader = m_GraphicsShaderLibrary.Get("Test");
        iGe::Renderer::Submit(shader, m_CubeVertexArray, m_ModelTransform);
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

void RuntimeLodLayer::LoadModel(std::string const& path) {
    // read file via assimp
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                                                           aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
    {
        IGE_ERROR("Assimp Error: {}", importer.GetErrorString());
        return;
    }

    // assuming the file has only one grid
    aiMesh* mesh = scene->mMeshes[0];

    std::vector<float> vertices;
    for (int i = 0; i < mesh->mNumVertices; ++i) {
        auto vertex = mesh->mVertices[i];
        vertices.push_back(vertex.x);
        vertices.push_back(vertex.y);
        vertices.push_back(vertex.z);
    }

    std::vector<uint32_t> indices;
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) { indices.push_back(face.mIndices[j]); }
    }

    m_VertexArray = iGe::VertexArray::Create();

    auto vertexBuffer = iGe::VertexBuffer::Create(vertices.data(), vertices.size() * sizeof(float));
    iGe::BufferLayout layout = {{iGe::ShaderDataType::Float3, "a_Position"}};
    vertexBuffer->SetLayout(layout);
    m_VertexArray->AddVertexBuffer(vertexBuffer);

    auto indexBuffer = iGe::IndexBuffer::Create(indices.data(), indices.size());
    m_VertexArray->SetIndexBuffer(indexBuffer);

    // update camera position
    glm::vec3 minBounds(FLT_MAX);
    glm::vec3 maxBounds(-FLT_MAX);

    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        const aiVector3D& v = mesh->mVertices[i];

        minBounds.x = std::min(minBounds.x, v.x);
        minBounds.y = std::min(minBounds.y, v.y);
        minBounds.z = std::min(minBounds.z, v.z);

        maxBounds.x = std::max(maxBounds.x, v.x);
        maxBounds.y = std::max(maxBounds.y, v.y);
        maxBounds.z = std::max(maxBounds.z, v.z);
    }

    glm::vec3 center = (minBounds + maxBounds) * 0.5f;
    float radius = glm::length(maxBounds - minBounds) * 0.5f;

    m_ModelCenter = center;
    m_CameraPosition = glm::vec3{center.x, center.y, center.z + 3.0f * radius};
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
            oldPoint3D.z = sqrt(rsqr - old_x2y2);
        } else {
            oldPoint3D.z = 0.5 * rsqr / sqrt(old_x2y2);
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
    double phi = 2.0 * asin(t);
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
