module;
#include "iGeMacro.h"

export module iGed.RuntimeLodLayer;
import iGe;

export class RuntimeLodLayer : public iGe::Layer {
public:
    RuntimeLodLayer();

    void OnUpdate(iGe::Timestep ts) override;
    void OnImGuiRender() override;
    void OnEvent(iGe::Event& event) override;

private:
    bool OnWindowResizeEvent(iGe::WindowResizeEvent& event);
    bool OnKeyPressedEvent(iGe::KeyPressedEvent& event);
    bool OnMouseScrolledEvent(iGe::MouseScrolledEvent& event);
    bool OnMouseButtonPresseddEvent(iGe::MouseButtonPressedEvent& event);
    bool OnMouseButtonReleasedEvent(iGe::MouseButtonReleasedEvent& event);
    bool OnMouseMoveEvent(iGe::MouseMoveEvent& event);

    void LoadModel(std::string const& path);
    void ModelRotation();
    void ViewTranslation();

    iGe::ShaderLibrary<iGe::GraphicsShader> m_GraphicsShaderLibrary;

    iGe::Ref<iGe::VertexArray> m_VertexArray;
    iGe::Ref<iGe::Texture2D> m_Texture;
    iGe::Ref<iGe::Buffer> m_CameraDataUniform;

    iGe::PerspectiveCamera m_Camera;
    glm::vec3 m_CameraPosition;
    float m_CameraMoveSpeed = 1.0f;
    float m_CameraRotation = 0.0f;
    float m_CameraRotationSpeed = 90.0f;

    glm::vec3 m_ModelCenter = glm::vec3{0.0f};
    glm::mat4 m_ModelTransform = glm::mat4{1.0f};

    glm::vec2 m_LastMousePosition = glm::vec2{0.0f};
};
