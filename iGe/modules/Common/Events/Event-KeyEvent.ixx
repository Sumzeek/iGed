module;
#include "iGeMacro.h"

export module iGe.Event:KeyEvent;
import :Event;
import :KeyCodes;

namespace iGe
{
class IGE_API KeyEvent : public Event {
public:
    inline iGeKey GetKeyCode() const { return m_KeyCode; }

    inline virtual uint32 GetCategoryFlags() const override { return EventCategoryInput | EventCategoryKeyboard; }

protected:
    KeyEvent(iGeKey keycode) : m_KeyCode{keycode} {}

    iGeKey m_KeyCode;
};

export class IGE_API KeyPressedEvent : public KeyEvent {
public:
    KeyPressedEvent(iGeKey keycode, int repeatCount) : KeyEvent{keycode}, m_RepeatCount{repeatCount} {}

    inline int32 GetRepeatCount() const { return m_RepeatCount; }

    inline std::string ToString() const override {
        return std::format("KeyPressedEvent: {0} ({1})", m_KeyCode, m_RepeatCount);
    }

    //EVENT_CLASS_TYPE(KeyPressed)
    inline static EventType GetStaticType() { return EventType::KeyPressed; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
    inline virtual const char* GetName() const override { return "KeyPressed"; }

private:
    int m_RepeatCount;
};

export class IGE_API KeyReleasedEvent : public KeyEvent {
public:
    KeyReleasedEvent(iGeKey keycode) : KeyEvent{keycode} {}

    inline std::string ToString() const override { return std::format("KeyReleasedEvent: {0}", m_KeyCode); }
    inline static EventType GetStaticType() { return EventType::KeyReleased; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
    inline virtual const char* GetName() const override { return "KeyReleased"; }
};

export class IGE_API KeyTypedEvent : public Event {
public:
    KeyTypedEvent(uint32 codepoint) : m_CodePoint{codepoint} {}

    inline uint32 GetCodePoint() const { return m_CodePoint; }

    inline std::string ToString() const override {
        return std::format("KeyTypedEvent: '{}'", std::string(1, static_cast<char32_t>(m_CodePoint)));
    }

    inline static EventType GetStaticType() { return EventType::KeyTyped; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
    inline virtual const char* GetName() const override { return "KeyTyped"; }
    inline virtual uint32 GetCategoryFlags() const override { return EventCategoryInput | EventCategoryKeyboard; }

private:
    uint32 m_CodePoint; // Unicode Point
};
} // namespace iGe
