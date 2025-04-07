module iGe.Event;
import std;

namespace iGe
{
/////////////////////////////////////////////////////////////////////////////
// KeyEvent /////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

KeyEvent::KeyEvent(iGeKey keycode) : m_KeyCode{keycode} {}

iGeKey KeyEvent::GetKeyCode() const { return m_KeyCode; }

int KeyEvent::GetCategoryFlags() const { return EventCategoryInput | EventCategoryKeyboard; }

/////////////////////////////////////////////////////////////////////////////
// KeyPressedEvent //////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

KeyPressedEvent::KeyPressedEvent(iGeKey keycode, int repeatCount) : KeyEvent{keycode}, m_RepeatCount{repeatCount} {}

int KeyPressedEvent::GetRepeatCount() const { return m_RepeatCount; }

std::string KeyPressedEvent::ToString() const {
    return std::format("KeyPressedEvent: {0} ({1})", m_KeyCode, m_RepeatCount);
}

EventType KeyPressedEvent::GetStaticType() { return EventType::KeyPressed; }

EventType KeyPressedEvent::GetEventType() const { return GetStaticType(); }

const char* KeyPressedEvent::GetName() const { return "KeyPressed"; }

/////////////////////////////////////////////////////////////////////////////
// KeyReleasedEvent /////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

KeyReleasedEvent::KeyReleasedEvent(iGeKey keycode) : KeyEvent{keycode} {}

std::string KeyReleasedEvent::ToString() const { return std::format("KeyReleasedEvent: {0}", m_KeyCode); }

EventType KeyReleasedEvent::GetStaticType() { return EventType::KeyReleased; }

EventType KeyReleasedEvent::GetEventType() const { return GetStaticType(); }

const char* KeyReleasedEvent::GetName() const { return "KeyReleased"; }

/////////////////////////////////////////////////////////////////////////////
// KeyTypedEvent ////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

KeyTypedEvent::KeyTypedEvent(unsigned int codepoint) : m_CodePoint{codepoint} {}

unsigned int KeyTypedEvent::GetCodePoint() const { return m_CodePoint; }

std::string KeyTypedEvent::ToString() const {
    return std::format("KeyTypedEvent: '{}'", std::string(1, static_cast<char32_t>(m_CodePoint)));
}

EventType KeyTypedEvent::GetStaticType() { return EventType::KeyTyped; }

EventType KeyTypedEvent::GetEventType() const { return GetStaticType(); }

const char* KeyTypedEvent::GetName() const { return "KeyTyped"; }

int KeyTypedEvent::GetCategoryFlags() const { return EventCategoryInput | EventCategoryKeyboard; }

} // namespace iGe