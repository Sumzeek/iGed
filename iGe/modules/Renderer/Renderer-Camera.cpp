module;
#include "iGeMacro.h"

module iGe.Renderer;
import :Camera;
import glm;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// Camera ///////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
const glm::vec3& Camera::GetPosition() const { return m_Position; }

void Camera::SetPosition(const glm::vec3& position) {
    m_Position = position;
    RecalculateViewMatrix();
}

float Camera::GetRotation() const { return m_Rotation; }

void Camera::SetRotation(float rotation) {
    m_Rotation = rotation;
    RecalculateViewMatrix();
}

/////////////////////////////////////////////////////////////////////////////
// OrthographicCamera ///////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
    : m_ProjectionMatrix(glm::gtc::ortho(left, right, bottom, top, -1.0f, 1.0f)) {
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void OrthographicCamera::SetProjection(float left, float right, float bottom, float top) {
    m_ProjectionMatrix = glm::gtc::ortho(left, right, bottom, top, -1.0f, 1.0f);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

const glm::mat4& OrthographicCamera::GetProjectionMatrix() const { return m_ProjectionMatrix; }

void OrthographicCamera::RecalculateViewMatrix() {
    glm::mat4 transform = glm::gtc::translate(glm::mat4(1.0f), m_Position) *
                          glm::gtc::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), glm::vec3(0, 0, 1));

    m_ViewMatrix = glm::inverse(transform); // equal to lookat[R * T]
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

/////////////////////////////////////////////////////////////////////////////
/// PerspectiveCamera ///////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
PerspectiveCamera::PerspectiveCamera(float fov, float aspectRatio, float nearClip, float farClip)
    : m_FOV(fov), m_AspectRatio(aspectRatio), m_NearClip(nearClip), m_FarClip(farClip), m_ProjectionMatrix{1.0f} {
    m_ProjectionMatrix = glm::gtc::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void PerspectiveCamera::SetProjection(float fov, float aspectRatio, float nearClip, float farClip) {
    m_FOV = fov;
    m_AspectRatio = aspectRatio;
    m_NearClip = nearClip;
    m_FarClip = farClip;

    m_ProjectionMatrix = glm::gtc::perspective(glm::radians(fov), aspectRatio, nearClip, farClip);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

const glm::mat4& PerspectiveCamera::GetProjectionMatrix() const { return m_ProjectionMatrix; }

void PerspectiveCamera::RecalculateViewMatrix() {
    glm::mat4 transform = glm::gtc::translate(glm::mat4(1.0f), m_Position) *
                          glm::gtc::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), glm::vec3(0, 0, 1));

    m_ViewMatrix = glm::inverse(transform); // equal to lookat[R * T]
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

} // namespace iGe