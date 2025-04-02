module iGe.Event;
import std;

namespace iGe
{
// ---------------------------------- MouseMoveEvent::Implementation ----------------------------------
MouseMoveEvent::MouseMoveEvent(float x, float y) : m_MouseX{x}, m_MouseY{y} {}

inline float MouseMoveEvent::GetX() const { return m_MouseX; }

inline float MouseMoveEvent::GetY() const { return m_MouseY; }

std::string MouseMoveEvent::ToString() const { return std::format("MouseMoveEvent: {0}, {1}", m_MouseX, m_MouseY); }

EventType MouseMoveEvent::GetStaticType() { return EventType::MouseMoved; }

EventType MouseMoveEvent::GetEventType() const { return GetStaticType(); }

const char* MouseMoveEvent::GetName() const { return "MouseMoved"; }

int MouseMoveEvent::GetCategoryFlags() const { return EventCategoryInput | EventCategoryMouse; }

// ---------------------------------- MouseScrolledEvent::Implementation ----------------------------------
MouseScrolledEvent::MouseScrolledEvent(float xOffset, float yOffset) : m_XOffset{xOffset}, m_YOffset{yOffset} {}

inline float MouseScrolledEvent::GetXOffset() const { return m_XOffset; }

inline float MouseScrolledEvent::GetYOffset() const { return m_YOffset; }

std::string MouseScrolledEvent::ToString() const {
    return std::format("MouseScrolledEvent: {0}, {1}", m_XOffset, m_YOffset);
}

//EVENT_CLASS_TYPE(MouseScrolled)
EventType MouseScrolledEvent::GetStaticType() { return EventType::MouseScrolled; }

EventType MouseScrolledEvent::GetEventType() const { return GetStaticType(); }

const char* MouseScrolledEvent::GetName() const { return "MouseScrolled"; }

//EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)
int MouseScrolledEvent::GetCategoryFlags() const { return EventCategoryInput | EventCategoryMouse; }

// ---------------------------------- MouseButtonEvent::Implementation ----------------------------------
MouseButtonEvent::MouseButtonEvent(iGeKey button) : m_Button{button} {}

iGeKey MouseButtonEvent::GetMouseButton() const { return m_Button; }

//EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)
int MouseButtonEvent::GetCategoryFlags() const { return EventCategoryInput | EventCategoryMouseButton; }

// ---------------------------------- MouseButtonPressedEvent::Implementation ----------------------------------
MouseButtonPressedEvent::MouseButtonPressedEvent(iGeKey button) : MouseButtonEvent{button} {}

std::string MouseButtonPressedEvent::ToString() const { return std::format("MousePressedEvent: {0}", m_Button); }

EventType MouseButtonPressedEvent::GetStaticType() { return EventType::MouseButtonPressed; }

EventType MouseButtonPressedEvent::GetEventType() const { return GetStaticType(); }

const char* MouseButtonPressedEvent::GetName() const { return "MouseButtonPressed"; }

// ---------------------------------- MouseButtonReleasedEvent::Implementation ----------------------------------
MouseButtonReleasedEvent::MouseButtonReleasedEvent(iGeKey button) : MouseButtonEvent{button} {}

std::string MouseButtonReleasedEvent::ToString() const { return std::format("MouseReleasedEvent: {0}", m_Button); }

EventType MouseButtonReleasedEvent::GetStaticType() { return EventType::MouseButtonReleased; }

EventType MouseButtonReleasedEvent::GetEventType() const { return GetStaticType(); }

const char* MouseButtonReleasedEvent::GetName() const { return "MouseButtonReleased"; }

} // namespace iGe