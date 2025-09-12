module;
#include "iGeMacro.h"

export module iGe.Event:MouseEvent;
import :Event;
import :KeyCodes;

namespace iGe
{
export class IGE_API MouseMoveEvent : public Event {
public:
    MouseMoveEvent(float32 x, float32 y) : m_MouseX{x}, m_MouseY{y} {}

    inline float32 GetX() const { return m_MouseX; }
    inline float32 GetY() const { return m_MouseY; }

    inline std::string ToString() const override { return std::format("MouseMoveEvent: {0}, {1}", m_MouseX, m_MouseY); }

    inline static EventType GetStaticType() { return EventType::MouseMoved; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
    inline virtual const char* GetName() const override { return "MouseMoved"; }
    inline virtual uint32 GetCategoryFlags() const override { return EventCategoryInput | EventCategoryMouse; }

private:
    float32 m_MouseX, m_MouseY;
};

export class IGE_API MouseScrolledEvent : public Event {
public:
    MouseScrolledEvent(float32 xOffset, float32 yOffset) : m_XOffset{xOffset}, m_YOffset{yOffset} {}

    inline float32 GetXOffset() const { return m_XOffset; }
    inline float32 GetYOffset() const { return m_YOffset; }

    inline std::string ToString() const override {
        return std::format("MouseScrolledEvent: {0}, {1}", m_XOffset, m_YOffset);
    }

    inline static EventType GetStaticType() { return EventType::MouseScrolled; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
    inline virtual const char* GetName() const override { return "MouseScrolled"; }
    inline virtual uint32 GetCategoryFlags() const override { return EventCategoryInput | EventCategoryMouse; }

private:
    float32 m_XOffset, m_YOffset;
};

class IGE_API MouseButtonEvent : public Event {
public:
    MouseButtonEvent(iGeKey button) : m_Button{button} {}

    inline iGeKey GetMouseButton() const { return m_Button; }

    inline virtual uint32 GetCategoryFlags() const override { return EventCategoryInput | EventCategoryMouseButton; }

protected:
    iGeKey m_Button;
};

export class IGE_API MouseButtonPressedEvent : public MouseButtonEvent {
public:
    MouseButtonPressedEvent(iGeKey button) : MouseButtonEvent{button} {}

    inline std::string ToString() const override { return std::format("MousePressedEvent: {0}", m_Button); }

    inline static EventType GetStaticType() { return EventType::MouseButtonPressed; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
    inline virtual const char* GetName() const override { return "MouseButtonPressed"; }
};

export class IGE_API MouseButtonReleasedEvent : public MouseButtonEvent {
public:
    MouseButtonReleasedEvent(iGeKey button) : MouseButtonEvent{button} {}

    inline std::string ToString() const override { return std::format("MouseReleasedEvent: {0}", m_Button); }

    static EventType GetStaticType() { return EventType::MouseButtonReleased; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "MouseButtonReleased"; }
};
} // namespace iGe
