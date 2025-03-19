module;
#include "Common/Core.h"
#include "Common/iGepch.h"

export module iGe.Event:Mouse;

import :Base;

namespace iGe
{

export class IGE_API MouseMoveEvent : public Event {
public:
    MouseMoveEvent(float x, float y);

    inline float GetX() const;
    inline float GetY() const;

    std::string ToString() const override;

    //EVENT_CLASS_TYPE(MouseMoved)
    static EventType GetStaticType();
    virtual EventType GetEventType() const override;
    virtual const char* GetName() const override;

    //EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)
    virtual int GetCategoryFlags() const override;

private:
    float m_MouseX, m_MouseY;
};

export class IGE_API MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(float xOffset, float yOffset);

    inline float GetXOffset() const;
    inline float GetYOffset() const;

    std::string ToString() const override;

    //EVENT_CLASS_TYPE(MouseScrolled)
    static EventType GetStaticType();
    virtual EventType GetEventType() const override;
    virtual const char* GetName() const override;

    //EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)
    virtual int GetCategoryFlags() const override;

private:
    float m_XOffset, m_YOffset;
};

class IGE_API MouseButtonEvent : public Event {
public:
    MouseButtonEvent(int button);

    inline float GetMouseButton() const;

    //EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)
    virtual int GetCategoryFlags() const override;

protected:
    int m_Button;
};

export class IGE_API MouseButtonPressedEvent : public MouseButtonEvent {
public:
    MouseButtonPressedEvent(int button);

    std::string ToString() const override;

    //EVENT_CLASS_TYPE(MouseButtonPressed)
    static EventType GetStaticType();
    virtual EventType GetEventType() const override;
    virtual const char* GetName() const override;
};

export class IGE_API MouseButtonReleasedEvent : public MouseButtonEvent {
public:
    MouseButtonReleasedEvent(int button);

    std::string ToString() const override;

    //EVENT_CLASS_TYPE(MouseButtonReleased)
    static EventType GetStaticType();
    virtual EventType GetEventType() const override;
    virtual const char* GetName() const override;
};

// ----------------- MouseMoveEvent::Implementation -----------------
MouseMoveEvent::MouseMoveEvent(float x, float y) : m_MouseX(x), m_MouseY(y) {}

inline float MouseMoveEvent::GetX() const { return m_MouseX; }
inline float MouseMoveEvent::GetY() const { return m_MouseY; }

std::string MouseMoveEvent::ToString() const { return std::format("MouseMoveEvent: {0}, {1}", m_MouseX, m_MouseY); }

EventType MouseMoveEvent::GetStaticType() { return EventType::MouseMoved; }
EventType MouseMoveEvent::GetEventType() const { return GetStaticType(); }
const char* MouseMoveEvent::GetName() const { return "MouseMoved"; }

int MouseMoveEvent::GetCategoryFlags() const { return EventCategoryInput | EventCategoryMouse; }

// ----------------- MouseScrolledEvent::Implementation -----------------
MouseScrolledEvent::MouseScrolledEvent(float xOffset, float yOffset) : m_XOffset(xOffset), m_YOffset(yOffset) {}

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

// ----------------- MouseButtonEvent::Implementation -----------------
MouseButtonEvent::MouseButtonEvent(int button) : m_Button(button) {}

float MouseButtonEvent::GetMouseButton() const { return m_Button; }

//EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)
int MouseButtonEvent::GetCategoryFlags() const { return EventCategoryInput | EventCategoryMouse; }

// ----------------- MouseButtonPressedEvent::Implementation -----------------
MouseButtonPressedEvent::MouseButtonPressedEvent(int button) : MouseButtonEvent(button) {}

std::string MouseButtonPressedEvent::ToString() const { return std::format("MousePressedEvent: {0}", m_Button); }

EventType MouseButtonPressedEvent::GetStaticType() { return EventType::MouseButtonPressed; }
EventType MouseButtonPressedEvent::GetEventType() const { return GetStaticType(); }
const char* MouseButtonPressedEvent::GetName() const { return "MouseButtonPressed"; }

// ----------------- MouseButtonReleasedEvent::Implementation -----------------
MouseButtonReleasedEvent::MouseButtonReleasedEvent(int button) : MouseButtonEvent(button) {}

std::string MouseButtonReleasedEvent::ToString() const { return std::format("MouseReleasedEvent: {0}", m_Button); }

EventType MouseButtonReleasedEvent::GetStaticType() { return EventType::MouseButtonReleased; }
EventType MouseButtonReleasedEvent::GetEventType() const { return GetStaticType(); }
const char* MouseButtonReleasedEvent::GetName() const { return "MouseButtonReleased"; }

} // namespace iGe
