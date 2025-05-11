#pragma once

import iGe;

class ExampleLayer : public iGe::Layer {
public:
    ExampleLayer();

    void OnUpdate(iGe::Timestep ts) override;
    void OnImGuiRender() override;
    void OnEvent(iGe::Event& event) override;

private:
    bool OnPressedEvent(iGe::KeyPressedEvent& event);

    iGe::ShaderLibrary m_ShaderLibrary;

    iGe::Ref<iGe::VertexArray> m_VertexArray;
    iGe::Ref<iGe::VertexArray> m_SquareVertexArray;
    iGe::Ref<iGe::Texture2D> m_Texture;
    iGe::Ref<iGe::Texture2D> m_iGameLogoTexture;

    iGe::OrthographicCamera m_Camera;
    glm::vec3 m_CameraPosition = glm::vec3{0.0f};
    float m_CameraMoveSpeed = 1.0f;
    float m_CameraRotation = 0.0f;
    float m_CameraRotationSpeed = 90.0f;
};
