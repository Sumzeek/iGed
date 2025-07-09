module;
#include "iGeMacro.h"

module iGe.Renderer;
import :OrthographicCamera;

import glm;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// OrthographicCamera ///////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
OrthographicCamera::OrthographicCamera(float left, float right, float bottom, float top)
    : m_ProjectionMatrix{glm::gtc::ortho(left, right, bottom, top, -1.0f, 1.0f)}, m_ViewMatrix{1.0f} {
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

void OrthographicCamera::SetProjection(float left, float right, float bottom, float top) {
    m_ProjectionMatrix = glm::gtc::ortho(left, right, bottom, top, -1.0f, 1.0f);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

const glm::vec3& OrthographicCamera::GetPosition() const { return m_Position; }

void OrthographicCamera::SetPosition(const glm::vec3& position) {
    m_Position = position;
    RecalculateViewMatrix();
}

float OrthographicCamera::GetRotation() const { return m_Rotation; }

void OrthographicCamera::SetRotation(float rotation) {
    m_Rotation = rotation;
    RecalculateViewMatrix();
}

const glm::mat4& OrthographicCamera::GetProjectionMatrix() const { return m_ProjectionMatrix; }

const glm::mat4& OrthographicCamera::GetViewMatrix() const { return m_ViewMatrix; }

const glm::mat4& OrthographicCamera::GetViewProjectionMatrix() const { return m_ViewProjectionMatrix; }

void OrthographicCamera::RecalculateViewMatrix() {
    glm::mat4 transform = glm::gtc::translate(glm::mat4(1.0f), m_Position) *
                          glm::gtc::rotate(glm::mat4(1.0f), glm::radians(m_Rotation), glm::vec3(0, 0, 1));

    // Move the camera as if it were an object.
    m_ViewMatrix = glm::inverse(transform);
    m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

} // namespace iGe