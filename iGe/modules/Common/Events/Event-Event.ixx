module;
#include "iGeMacro.h"

export module iGe.Event:Event;
import iGe.Types;

namespace iGe
{

export enum class EventType : int {
    None = 0,
    WindowClose,
    WindowResize,
    WindowFocus,
    WindowLostFocus,
    WindowMoved,
    AppTick,
    AppUpdate,
    AppRender,
    KeyPressed,
    KeyReleased,
    KeyTyped,
    MouseButtonPressed,
    MouseButtonReleased,
    MouseMoved,
    MouseScrolled
};

export enum EventCategory {
    None = 0,
    EventCategoryApplication = 1 << 0,
    EventCategoryInput = 1 << 1,
    EventCategoryKeyboard = 1 << 2,
    EventCategoryMouse = 1 << 3,
    EventCategoryMouseButton = 1 << 4,
};

export class IGE_API Event {
public:
    virtual ~Event() = default;

    bool m_Handled = false;

    virtual EventType GetEventType() const = 0;
    virtual const char* GetName() const = 0;
    virtual uint32 GetCategoryFlags() const = 0;
    virtual string ToString() const { return GetName(); }

    bool IsInCategory(EventCategory category) { return GetCategoryFlags() & category; }
};

export class IGE_API EventDispatcher {
public:
    EventDispatcher(Event& event) : m_Event(event) {}

    template<class T>
    bool Dispatch(std::function<bool(T&)> func) {
        if (m_Event.GetEventType() == T::GetStaticType()) {
            m_Event.m_Handled = func(*(T*) &m_Event);
            return true;
        }
        return false;
    }

private:
    Event& m_Event;
};

// ---------------------------------- Other::Implementation ----------------------------------
export std::ostream& operator<<(std::ostream& os, const Event& e) { return os << e.ToString(); }
export string format_as(const Event& e) { return e.ToString(); }

} // namespace iGe

template<>
struct std::formatter<iGe::Event> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const iGe::Event& e, format_context& ctx) const { return format_to(ctx.out(), "{}", e.ToString()); }
};
