module;
#include "Macro.h"

export module iGe.Event:Key;
import std;
import :Base;
import :KeyCodes;

namespace iGe
{

class IGE_API KeyEvent : public Event {
public:
    inline iGeKey GetKeyCode() const;

    //EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryKeyboard)
    virtual int GetCategoryFlags() const override;

protected:
    KeyEvent(iGeKey keycode);

    iGeKey m_KeyCode;
};

export class IGE_API KeyPressedEvent : public KeyEvent {
public:
    KeyPressedEvent(iGeKey keycode, int repeatCount);

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
    KeyReleasedEvent(iGeKey keycode);

    std::string ToString() const override;

    //EVENT_CLASS_TYPE(KeyReleased)
    static EventType GetStaticType();
    virtual EventType GetEventType() const override;
    virtual const char* GetName() const override;
};

export class IGE_API KeyTypedEvent : public Event {
public:
    KeyTypedEvent(unsigned int codepoint);

    inline unsigned int GetCodePoint() const;

    std::string ToString() const override;

    //EVENT_CLASS_TYPE(KeyReleased)
    static EventType GetStaticType();
    virtual EventType GetEventType() const override;
    virtual const char* GetName() const override;

    //EVENT_CLASS_CATEGORY(EventCategoryInput | EventCategoryKeyboard)
    virtual int GetCategoryFlags() const override;

private:
    unsigned int m_CodePoint; // Unicode Point
};

} // namespace iGe
