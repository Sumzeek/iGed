module iGe.Renderer;
import :OrthographicCamera;

namespace iGe
{

// =================================================================================================
// OrthographicCamera
// =================================================================================================

OrthographicCamera::OrthographicCamera(float32 left, float32 right, float32 bottom, float32 top)
    : m_ProjectionMatrix{glm::gtc::orthoRH_ZO(left, right, bottom, top, -1.0f, 1.0f)}, m_ViewMatrix{1.0f} {
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void OrthographicCamera::SetProjection(float32 left, float32 right, float32 bottom, float32 top) {
    m_ProjectionMatrix = glm::gtc::orthoRH_ZO(left, right, bottom, top, -1.0f, 1.0f);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void OrthographicCamera::SetPosition(const glm::vec3& position) {
    m_Position = position;
    RecalculateViewMatrix();
}

void OrthographicCamera::SetRotation(float32 rotation) {
    m_Rotation = rotation;
    RecalculateViewMatrix();
}

void OrthographicCamera::RecalculateViewMatrix() {
    glm::mat4 transform = glm::gtc::translate(glm::mat4(1.0f), m_Position) *
                          glm::gtc::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), glm::vec3(0, 0, 1));

    // Move the camera as if it were an object.
    m_ViewMatrix = glm::inverse(transform);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

} // namespace iGe
