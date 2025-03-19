module;
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
    virtual int GetCategoryFlags() const override;

protected:
    KeyEvent(int keycode);

    int m_KeyCode;
};

export class IGE_API KeyPressedEvent : public KeyEvent {
public:
    KeyPressedEvent(int keycode, int repeatCount);

    inline int GetRepeatCount() const;

    std::string ToString() const override;

    //EVENT_CLASS_TYPE(KeyPressed)
    static EventType GetStaticType();
    virtual EventType GetEventType() const override;
    virtual const char* GetName() const override;

private:
    int m_RepeatCount;
};

export class IGE_API KeyReleasedEvent : public KeyEvent {
public:
    KeyReleasedEvent(int keycode);

    std::string ToString() const override;

    //EVENT_CLASS_TYPE(KeyReleased)
    static EventType GetStaticType();
    virtual EventType GetEventType() const override;
    virtual const char* GetName() const override;
};

// ----------------- KeyEvent::Implementation -----------------
KeyEvent::KeyEvent(int keycode) : m_KeyCode(keycode) {}
int KeyEvent::GetKeyCode() const { return m_KeyCode; }

int KeyEvent::GetCategoryFlags() const { return EventCategoryInput | EventCategoryKeyboard; }

// ----------------- KeyPressedEvent::Implementation -----------------
KeyPressedEvent::KeyPressedEvent(int keycode, int repeatCount) : KeyEvent(keycode), m_RepeatCount(repeatCount) {}
int KeyPressedEvent::GetRepeatCount() const { return m_RepeatCount; }
std::string KeyPressedEvent::ToString() const {
    return std::format("KeyPressedEvent: {0} ({1})", m_KeyCode, m_RepeatCount);
}

EventType KeyPressedEvent::GetStaticType() { return EventType::KeyPressed; }
EventType KeyPressedEvent::GetEventType() const { return GetStaticType(); }
const char* KeyPressedEvent::GetName() const { return "KeyPressed"; }

// ----------------- KeyReleasedEvent::Implementation -----------------
KeyReleasedEvent::KeyReleasedEvent(int keycode) : KeyEvent(keycode) {}
std::string KeyReleasedEvent::ToString() const { return std::format("KeyReleasedEvent: {0}", m_KeyCode); }

EventType KeyReleasedEvent::GetStaticType() { return EventType::KeyReleased; }
EventType KeyReleasedEvent::GetEventType() const { return GetStaticType(); }
const char* KeyReleasedEvent::GetName() const { return "KeyReleased"; }

} // namespace iGe
