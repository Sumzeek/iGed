#include "Macro.h"

module iGe.Event;
import std;

namespace iGe
{
// ---------------------------------- Event::Implementation ----------------------------------
std::string Event::ToString() const { return GetName(); }

// ---------------------------------- EventDispatcher::Implementation ----------------------------------
EventDispatcher::EventDispatcher(Event& event) : m_Event(event) {}

// ---------------------------------- KeyEvent::Implementation ----------------------------------
KeyEvent::KeyEvent(iGeKey keycode) : m_KeyCode{keycode} {}

iGeKey KeyEvent::GetKeyCode() const { return m_KeyCode; }

int KeyEvent::GetCategoryFlags() const { return EventCategoryInput | EventCategoryKeyboard; }

// ---------------------------------- KeyPressedEvent::Implementation ----------------------------------
KeyPressedEvent::KeyPressedEvent(iGeKey keycode, int repeatCount) : KeyEvent{keycode}, m_RepeatCount{repeatCount} {}

int KeyPressedEvent::GetRepeatCount() const { return m_RepeatCount; }

std::string KeyPressedEvent::ToString() const {
    return std::format("KeyPressedEvent: {0} ({1})", m_KeyCode, m_RepeatCount);
}

EventType KeyPressedEvent::GetStaticType() { return EventType::KeyPressed; }

EventType KeyPressedEvent::GetEventType() const { return GetStaticType(); }

const char* KeyPressedEvent::GetName() const { return "KeyPressed"; }

// ---------------------------------- KeyReleasedEvent::Implementation ----------------------------------
KeyReleasedEvent::KeyReleasedEvent(iGeKey keycode) : KeyEvent{keycode} {}

std::string KeyReleasedEvent::ToString() const { return std::format("KeyReleasedEvent: {0}", m_KeyCode); }

EventType KeyReleasedEvent::GetStaticType() { return EventType::KeyReleased; }

EventType KeyReleasedEvent::GetEventType() const { return GetStaticType(); }

const char* KeyReleasedEvent::GetName() const { return "KeyReleased"; }

// ---------------------------------- KeyTypedEvent::Implementation ----------------------------------
KeyTypedEvent::KeyTypedEvent(unsigned int codepoint) : m_CodePoint{codepoint} {}

unsigned int KeyTypedEvent::GetCodePoint() const { return m_CodePoint; }

std::string KeyTypedEvent::ToString() const {
    return std::format("KeyTypedEvent: '{}'", std::string(1, static_cast<char32_t>(m_CodePoint)));
}

EventType KeyTypedEvent::GetStaticType() { return EventType::KeyTyped; }

EventType KeyTypedEvent::GetEventType() const { return GetStaticType(); }

const char* KeyTypedEvent::GetName() const { return "KeyTyped"; }

int KeyTypedEvent::GetCategoryFlags() const { return EventCategoryInput | EventCategoryKeyboard; }

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

// ---------------------------------- WindowResizeEvent::Implementation ----------------------------------
WindowResizeEvent::WindowResizeEvent(unsigned int width, unsigned int height) : m_Width{width}, m_Height{height} {}

unsigned int WindowResizeEvent::GetWidth() const { return m_Width; }

unsigned int WindowResizeEvent::GetHeight() const { return m_Height; }

std::string WindowResizeEvent::ToString() const {
    return std::format("WindowResizeEvent: {0}, {1}", m_Width, m_Height);
}

//EVENT_CLASS_TYPE(WindowResize)
EventType WindowResizeEvent::GetStaticType() { return EventType::WindowResize; }

EventType WindowResizeEvent::GetEventType() const { return GetStaticType(); }

const char* WindowResizeEvent::GetName() const { return "WindowResize"; }

//EVENT_CLASS_CATEGORY(EventCategoryApplication)
int WindowResizeEvent::GetCategoryFlags() const { return EventCategoryApplication; }

// ---------------------------------- WindowCloseEvent::Implementation ----------------------------------
WindowCloseEvent::WindowCloseEvent() {}

EventType WindowCloseEvent::GetStaticType() { return EventType::WindowClose; }

EventType WindowCloseEvent::GetEventType() const { return GetStaticType(); }

const char* WindowCloseEvent::GetName() const { return "WindowClose"; }

int WindowCloseEvent::GetCategoryFlags() const { return EventCategoryApplication; }

// ---------------------------------- AppTickEvent::Implementation ----------------------------------
AppTickEvent::AppTickEvent() {}

EventType AppTickEvent::GetStaticType() { return EventType::AppTick; }

EventType AppTickEvent::GetEventType() const { return GetStaticType(); }

const char* AppTickEvent::GetName() const { return "AppTick"; }

int AppTickEvent::GetCategoryFlags() const { return EventCategoryApplication; }

// ---------------------------------- AppUpdateEvent::Implementation ----------------------------------
AppUpdateEvent::AppUpdateEvent() {}

EventType AppUpdateEvent::GetStaticType() { return EventType::AppUpdate; }

EventType AppUpdateEvent::GetEventType() const { return GetStaticType(); }

const char* AppUpdateEvent::GetName() const { return "AppUpdate"; }

int AppUpdateEvent::GetCategoryFlags() const { return EventCategoryApplication; }

// ---------------------------------- AppRenderEvent::Implementation ----------------------------------
AppRenderEvent::AppRenderEvent() {}

EventType AppRenderEvent::GetStaticType() { return EventType::AppRender; }

EventType AppRenderEvent::GetEventType() const { return GetStaticType(); }

const char* AppRenderEvent::GetName() const { return "AppRender"; }

int AppRenderEvent::GetCategoryFlags() const { return EventCategoryApplication; }

} // namespace iGe
