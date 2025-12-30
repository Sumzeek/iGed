export module iGed.ExampleLayer;
import iGe;

export class ExampleLayer : public iGe::Layer {
public:
    ExampleLayer();

    void OnUpdate(iGe::Timestep ts) override;
    void OnImGuiRender() override;
    void OnEvent(iGe::Event& event) override;

private:
    bool OnPressedEvent(iGe::KeyPressedEvent& event);
    bool OnWindowResizeEvent(iGe::WindowResizeEvent& event);

    void CreateCommandPool();
    void CreateRenderPass();
    void CreatePipelineLayout();
    void CreateGraphicsPipeline();
    void CreateBuffers();
    void CreateDescriptorResources();
    void CreateDepthResources(uint32 width, uint32 height);

    iGe::Scope<iGe::RHICommandPool> m_CommandPool;
    iGe::Scope<iGe::RHICommandList> m_CommandList;

    iGe::Scope<iGe::RHIVertexBuffer> m_TriVertexBuffer;
    iGe::Scope<iGe::RHIIndexBuffer> m_TriIndexBuffer;
    iGe::Scope<iGe::RHIGraphicsPipeline> m_TriGraphicsPipeline;

    iGe::Scope<iGe::RHIVertexBuffer> m_QuadVertexBuffer;
    iGe::Scope<iGe::RHIIndexBuffer> m_QuadIndexBuffer;
    iGe::Scope<iGe::RHIGraphicsPipeline> m_QuadGraphicsPipeline;

    iGe::Scope<iGe::RHIUniformBuffer> m_UniformBuffer;
    iGe::Scope<iGe::RHIRenderPass> m_RenderPass;

    // Attachments
    iGe::Scope<iGe::RHITexture> m_ColorAttachment;
    iGe::Scope<iGe::RHITexture> m_Texture;
    iGe::Scope<iGe::RHITextureView> m_TextureView;
    iGe::Scope<iGe::RHITexture> m_DepthAttachment;
    iGe::Scope<iGe::RHITextureView> m_DepthTextureView;

    // Descriptor Set resources
    iGe::Scope<iGe::RHIDescriptorPool> m_DescriptorPool;
    iGe::Scope<iGe::RHIDescriptorSetLayout> m_DescriptorSetLayout;
    iGe::Scope<iGe::RHIDescriptorSet> m_DescriptorSet;
    iGe::Scope<iGe::RHIPipelineLayout> m_PipelineLayout;
    iGe::Scope<iGe::RHISampler> m_Sampler;

    // Triangle-specific descriptor resources (UBO only)
    iGe::Scope<iGe::RHIDescriptorSetLayout> m_TriDescriptorSetLayout;
    iGe::Scope<iGe::RHIDescriptorSet> m_TriDescriptorSet;
    iGe::Scope<iGe::RHIPipelineLayout> m_TriPipelineLayout;

    iGe::OrthographicCamera m_Camera;
    glm::vec3 m_CameraPosition = glm::vec3{0.0f};
    float32 m_CameraMoveSpeed = 1.0f;
    float32 m_CameraRotation = 0.0f;
    float32 m_CameraRotationSpeed = 90.0f;
};
