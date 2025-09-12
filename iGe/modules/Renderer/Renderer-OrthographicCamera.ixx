module;
#include "iGeMacro.h"

export module iGe.Renderer:OrthographicCamera;
import iGe.Common;
import glm;

namespace iGe
{
export class IGE_API OrthographicCamera {
public:
    OrthographicCamera(float32 left, float32 right, float32 bottom, float32 top);

    void SetProjection(float32 left, float32 right, float32 bottom, float32 top);

    inline const glm::vec3& GetPosition() const { return m_Position; }
    void SetPosition(const glm::vec3& position);

    inline float32 GetRotation() const { return m_Rotation; }
    void SetRotation(float32 rotation);

    inline const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
    inline const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
    inline const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

private:
    void RecalculateViewMatrix();

    glm::mat4 m_ViewMatrix;
    glm::mat4 m_ProjectionMatrix;
    glm::mat4 m_ViewProjectionMatrix;

    glm::vec3 m_Position = {0.0f, 0.0f, 0.0f};
    float32 m_Rotation = 0.0f;
};
} // namespace iGe
