module;
#include "iGeMacro.h"

export module iGed.RuntimeLodLayer;
import iGe;
import MeshFitting;

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

    void ModelRotation();
    void ViewTranslation();

    void Tessllation();
    void SoftwareRasterization();

    iGe::ShaderLibrary<iGe::GraphicsShader> m_GraphicsShaderLibrary;
    iGe::ShaderLibrary<iGe::ComputeShader> m_ComputeShaderLibrary;

    // Tesslation
    struct TessData {
        glm::uvec2 ScreenSize;
        std::uint32_t TriSize;
        std::uint32_t _padding_TriSize;
    };
    iGe::Scope<TessData> m_TessData;
    iGe::Ref<iGe::Buffer> m_TessDataUniform;

    iGe::Ref<iGe::Texture2D> m_DepthBuffer;

    // Model lod
    struct PerFrameData {
        glm::vec3 ViewPos;
        float DisplaceMapScale;
        glm::mat4 NormalMatrix;
    };
    iGe::Scope<PerFrameData> m_PerFrameData;
    iGe::Ref<iGe::Buffer> m_PerFrameDataUniform;

    MeshFitting::Mesh m_Model;
    iGe::Ref<iGe::Buffer> m_VertexBuffer;
    iGe::Ref<iGe::Buffer> m_IndexBuffer;
    iGe::Ref<iGe::Buffer> m_TessFactorBuffer;
    iGe::Ref<iGe::Buffer> m_CounterBuffer;
    iGe::Ref<iGe::VertexArray> m_ModelVertexArray;
    iGe::Ref<iGe::Texture2D> m_ModelNormalMap;
    iGe::Ref<iGe::Texture2D> m_ModelDisplaceMap;
    float m_DisplaceMapScale = 2.99f;
    //float m_DisplaceMapScale = 0.07281857f;

    // Camera
    iGe::PerspectiveCamera m_Camera;
    glm::vec3 m_CameraPosition;
    float m_CameraMoveSpeed = 1.0f;
    float m_CameraRotation = 0.0f;
    float m_CameraRotationSpeed = 90.0f;

    // Scene Data
    glm::vec3 m_ModelCenter = glm::vec3{0.0f};
    float m_ModelRadius = 1.0f;
    glm::mat4 m_ModelTransform = glm::mat4{1.0f};

    glm::vec2 m_LastMousePosition = glm::vec2{0.0f};
};
