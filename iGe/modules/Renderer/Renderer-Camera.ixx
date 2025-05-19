module;
#include "iGeMacro.h"

export module iGe.Renderer:Camera;
import glm;

namespace iGe
{

export class IGE_API Camera {
public:
    virtual ~Camera() = default;

    const glm::vec3& GetPosition() const;
    void SetPosition(const glm::vec3& position);

    float GetRotation() const;
    void SetRotation(float rotation);

    virtual const glm::mat4& GetProjectionMatrix() const = 0;
    const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
    const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

protected:
    virtual void RecalculateViewMatrix() = 0;

    glm::mat4 m_ViewMatrix = glm::mat4(1.0f);
    glm::mat4 m_ViewProjectionMatrix = glm::mat4(1.0f);

    glm::vec3 m_Position = glm::vec3(0.0f, 0.0f, 0.0f);
    float m_Rotation = 0.0f;
};

export class IGE_API OrthographicCamera : public Camera {
public:
    OrthographicCamera(float left, float right, float bottom, float top);

    void SetProjection(float left, float right, float bottom, float top);
    const glm::mat4& GetProjectionMatrix() const override;

protected:
    void RecalculateViewMatrix() override;

    glm::mat4 m_ProjectionMatrix;
};

export class IGE_API PerspectiveCamera : public Camera {
public:
    PerspectiveCamera(float fov, float aspectRatio, float nearClip, float farClip);

    void SetProjection(float fov, float aspectRatio, float nearClip, float farClip);
    const glm::mat4& GetProjectionMatrix() const override;

protected:
    void RecalculateViewMatrix() override;

    float m_FOV;
    float m_AspectRatio;
    float m_NearClip;
    float m_FarClip;
    glm::mat4 m_ProjectionMatrix;
};

} // namespace iGe
