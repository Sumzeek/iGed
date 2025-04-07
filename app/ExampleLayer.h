#pragma once

#include "iGeEntryPoint.h"

class ExampleLayer : public iGe::Layer {
public:
    ExampleLayer();

    void OnUpdate() override;
    void OnImGuiRender() override;
    void OnEvent(iGe::Event& event) override;

private:
    bool OnPressedEvent(iGe::KeyPressedEvent& event);

    iGe::Ref<iGe::VertexArray> m_VertexArray;
    iGe::Ref<iGe::VertexBuffer> m_VertexBuffer;
    iGe::Ref<iGe::IndexBuffer> m_IndexBuffer;
    iGe::Ref<iGe::Shader> m_Shader;
    iGe::OrthographicCamera m_Camera;

    glm::vec3 m_CameraPosition = glm::vec3{0.0f};
    float m_CameraMoveSpeed = 0.1f;
    float m_CameraRotation = 0.0f;
    float m_CameraRotationSpeed = 2.0f;
};

class Sandbox : public iGe::Application {
public:
    Sandbox();
    ~Sandbox() override;
};

namespace iGe
{
Application* CreateApplication() { return new Sandbox(); }
} // namespace iGe
