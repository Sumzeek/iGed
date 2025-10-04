module;
#include "iGeMacro.h"

export module iGed.RuntimeLodLayer;
import iGe;
import MeshBaker;

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

    iGe::ShaderLibrary<iGe::GraphicsShader> m_GraphicsShaderLibrary;
    iGe::ShaderLibrary<iGe::ComputeShader> m_ComputeShaderLibrary;

    struct PerFrameData {
        glm::vec3 ViewPos;
        float _padding_ViewPos;
        glm::mat4 NormalMatrix;
    };
    iGe::Scope<PerFrameData> m_PerFrameData;
    iGe::Ref<iGe::Buffer> m_PerFrameDataUniform;

    // Empty VAO
    iGe::Ref<iGe::VertexArray> m_EmptyVertexArray;

    // Test
    MeshBaker::Mesh m_SimModel;
    iGe::Ref<iGe::VertexArray> m_SimModelVertexArray;
    MeshBaker::Mesh m_OriModel;
    iGe::Ref<iGe::VertexArray> m_OriModelVertexArray;

    // Model
    MeshBaker::Mesh m_Model;
    iGe::Ref<iGe::VertexArray> m_ModelVertexArray;

    // Software Tessellation
    struct TessellatorData {
        glm::uvec2 ScreenSize;
        std::uint32_t TriSize;
        float DisplaceMapScale;
    };
    iGe::Scope<TessellatorData> m_TessellatorData;
    iGe::Ref<iGe::Buffer> m_TessellatorDataUniform;
    static constexpr std::uint32_t kMaxLodLevel = 4;
    iGe::Ref<iGe::Buffer> m_VertexBuffer;
    iGe::Ref<iGe::Buffer> m_IndexBuffer;
    iGe::Ref<iGe::Buffer> m_CounterBuffer;
    iGe::Ref<iGe::Buffer> m_SubBufferIn;
    iGe::Ref<iGe::Buffer> m_SubBufferCounter;
    iGe::Ref<iGe::Buffer> m_SubBufferOut;

    // Software Rasterization
    iGe::Ref<iGe::Texture2D> m_DepthBuffer;
    iGe::Ref<iGe::Buffer> m_Packed64Buffer;

    //iGe::Ref<iGe::Buffer> m_TessFactorBuffer;
    iGe::Ref<iGe::Texture2D> m_ModelDisplaceMap;
    float m_DisplaceMapScale = 1.0f;
    uint32_t m_TargetTessFactor = 5;

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
