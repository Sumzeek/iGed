#include "Common/Core.h"
#include "Common/iGepch.h"

export module iGe.Event:Key;

import :Base;

namespace iGe
{

class IGE_API KeyEvent : public Event {
public:
    inline int GetKeyCode() const;

    //EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryKeyboard)
    virtual int GetCategoryFlags() const override { return EventCategoryInput | EventCategoryKeyboard; }

protected:
    KeyEvent(int keycode);

    int m_KeyCode;
};

KeyEvent::KeyEvent(int keycode) : m_KeyCode(keycode) {}
int KeyEvent::GetKeyCode() const { return m_KeyCode; }

export class IGE_API KeyPressedEvent : public KeyEvent {
public:
    KeyPressedEvent(int keycode, int repeatCount);

    inline int GetRepeatCount() const;

    std::string ToString() const override;

    //EVENT_CLASS_TYPE(KeyPressed)
    static EventType GetStaticType() { return EventType::KeyPressed; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "KeyPressed"; }

private:
    int m_RepeatCount;
};

KeyPressedEvent::KeyPressedEvent(int keycode, int repeatCount) : KeyEvent(keycode), m_RepeatCount(repeatCount) {}
int KeyPressedEvent::GetRepeatCount() const { return m_RepeatCount; }
std::string KeyPressedEvent::ToString() const {
    return std::format("KeyPressedEvent: {0} ({1})", m_KeyCode, m_RepeatCount);
}

export class IGE_API KeyReleasedEvent : public KeyEvent {
public:
    KeyReleasedEvent(int keycode);

    std::string ToString() const override;

    //EVENT_CLASS_TYPE(KeyReleased)
    static EventType GetStaticType() { return EventType::KeyReleased; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "KeyReleased"; }
};

KeyReleasedEvent::KeyReleasedEvent(int keycode) : KeyEvent(keycode) {}
std::string KeyReleasedEvent::ToString() const { return std::format("KeyReleasedEvent: {0}", m_KeyCode); }

} // namespace iGe
