module;
#include "iGeMacro.h"

export module iGe.Event:KeyEvent;
import :Event;
import :KeyCodes;

namespace iGe
{

class IGE_API KeyEvent : public Event {
public:
    iGeKey GetKeyCode() const { return m_KeyCode; }

    virtual uint32 GetCategoryFlags() const override { return EventCategoryInput | EventCategoryKeyboard; }

protected:
    KeyEvent(iGeKey keycode) : m_KeyCode{keycode} {}

    iGeKey m_KeyCode;
};

export class IGE_API KeyPressedEvent : public KeyEvent {
public:
    KeyPressedEvent(iGeKey keycode, int repeatCount) : KeyEvent{keycode}, m_RepeatCount{repeatCount} {}

    int32 GetRepeatCount() const { return m_RepeatCount; }

    string ToString() const override { return std::format("KeyPressedEvent: {0} ({1})", m_KeyCode, m_RepeatCount); }

    //EVENT_CLASS_TYPE(KeyPressed)
    static EventType GetStaticType() { return EventType::KeyPressed; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "KeyPressed"; }

private:
    int m_RepeatCount;
};

export class IGE_API KeyReleasedEvent : public KeyEvent {
public:
    KeyReleasedEvent(iGeKey keycode) : KeyEvent{keycode} {}

    string ToString() const override { return std::format("KeyReleasedEvent: {0}", m_KeyCode); }
    static EventType GetStaticType() { return EventType::KeyReleased; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "KeyReleased"; }
};

export class IGE_API KeyTypedEvent : public Event {
public:
    KeyTypedEvent(uint32 codepoint) : m_CodePoint{codepoint} {}

    uint32 GetCodePoint() const { return m_CodePoint; }

    string ToString() const override {
        return std::format("KeyTypedEvent: '{}'", string(1, static_cast<char32_t>(m_CodePoint)));
    }

    static EventType GetStaticType() { return EventType::KeyTyped; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "KeyTyped"; }
    virtual uint32 GetCategoryFlags() const override { return EventCategoryInput | EventCategoryKeyboard; }

private:
    uint32 m_CodePoint; // Unicode Point
};

} // namespace iGe
