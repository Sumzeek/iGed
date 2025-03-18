#include "Common/Core.h"
#include "Common/iGepch.h"

export module iGe.Event:Mouse;

import :Base;

namespace iGe
{

export class IGE_API MouseMoveEvent : public Event {
public:
    MouseMoveEvent(float x, float y) : m_MouseX(x), m_MouseY(y) {}

    inline float GetX() const { return m_MouseX; }
    inline float GetY() const { return m_MouseY; }

    std::string ToString() const override { return std::format("MouseMoveEvent: {0}, {1}", m_MouseX, m_MouseY); }

    //EVENT_CLASS_TYPE(MouseMoved)
    static EventType GetStaticType() { return EventType::MouseMoved; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "MouseMoved"; }

    //EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)
    virtual int GetCategoryFlags() const override { return EventCategoryInput | EventCategoryMouse; }

private:
    float m_MouseX, m_MouseY;
};

export class IGE_API MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(float xOffset, float yOffset) : m_XOffset(xOffset), m_YOffset(yOffset) {}

    inline float GetXOffset() const { return m_XOffset; }
    inline float GetYOffset() const { return m_YOffset; }

    std::string ToString() const override { return std::format("MouseScrolledEvent: {0}, {1}", m_XOffset, m_YOffset); }

    //EVENT_CLASS_TYPE(MouseScrolled)
    static EventType GetStaticType() { return EventType::MouseScrolled; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "MouseScrolled"; }

    //EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)
    virtual int GetCategoryFlags() const override { return EventCategoryInput | EventCategoryMouse; }


private:
    float m_XOffset, m_YOffset;
};

class IGE_API MouseButtonEvent : public Event {
public:
    MouseButtonEvent(int button) : m_Button(button) {}

    inline float GetMouseButton() const { return m_Button; }

    //EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)
    virtual int GetCategoryFlags() const override { return EventCategoryInput | EventCategoryMouse; }

protected:
    int m_Button;
};

export class IGE_API MouseButtonPressedEvent : public MouseButtonEvent {
public:
    MouseButtonPressedEvent(int button) : MouseButtonEvent(button) {}

    std::string ToString() const override { return std::format("MousePressedEvent: {0}", m_Button); }

    //EVENT_CLASS_TYPE(MouseButtonPressed)
    static EventType GetStaticType() { return EventType::MouseButtonPressed; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "MouseButtonPressed"; }
};

export class IGE_API MouseButtonReleasedEvent : public MouseButtonEvent {
public:
    MouseButtonReleasedEvent(int button) : MouseButtonEvent(button) {}

    std::string ToString() const override { return std::format("MouseReleasedEvent: {0}", m_Button); }

    //EVENT_CLASS_TYPE(MouseButtonReleased)
    static EventType GetStaticType() { return EventType::MouseButtonReleased; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "MouseButtonReleased"; }
};

} // namespace iGe
