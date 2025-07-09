module;
#include "iGeMacro.h"

export module iGe.Event:MouseEvent;
import :Event;
import :KeyCodes;

import std;

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
    MouseButtonEvent(iGeKey button);

    inline iGeKey GetMouseButton() const;

    //EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryMouse)
    virtual int GetCategoryFlags() const override;

protected:
    iGeKey m_Button;
};

export class IGE_API MouseButtonPressedEvent : public MouseButtonEvent {
public:
    MouseButtonPressedEvent(iGeKey button);

    std::string ToString() const override;

    //EVENT_CLASS_TYPE(MouseButtonPressed)
    static EventType GetStaticType();
    virtual EventType GetEventType() const override;
    virtual const char* GetName() const override;
};

export class IGE_API MouseButtonReleasedEvent : public MouseButtonEvent {
public:
    MouseButtonReleasedEvent(iGeKey button);

    std::string ToString() const override;

    //EVENT_CLASS_TYPE(MouseButtonReleased)
    static EventType GetStaticType();
    virtual EventType GetEventType() const override;
    virtual const char* GetName() const override;
};

} // namespace iGe
