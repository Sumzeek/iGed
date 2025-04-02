module;
#include "iGeMacro.h"

export module iGe.Event:Event;
import std;

namespace iGe
{

export enum class EventType {
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
    EventCategoryApplication = BIT(0),
    EventCategoryInput = BIT(1),
    EventCategoryKeyboard = BIT(2),
    EventCategoryMouse = BIT(3),
    EventCategoryMouseButton = BIT(4),
};

//#define EVENT_CLASS_TYPE(type)                                                                                         \
//    static EventType GetStaticType() { return EventType::type; }                                                       \
//    virtual EventType GetEventType() const override { return GetStaticType(); }                                        \
//    virtual const char* GetName() const override { return #type; }
//
//#define EVENT_CLASS_CATEGORY(category)                                                                                 \
//    virtual int GetCategoryFlags() const override { return category; }

export class IGE_API Event {
public:
    virtual ~Event() = default;

    bool m_Handled = false;

    virtual EventType GetEventType() const = 0;
    virtual const char* GetName() const = 0;
    virtual int GetCategoryFlags() const = 0;
    virtual std::string ToString() const;

    bool IsInCategory(EventCategory category);
};

export class IGE_API EventDispatcher {
public:
    EventDispatcher(Event& event);

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
export std::string format_as(const Event& e) { return e.ToString(); }

} // namespace iGe

template<>
struct std::formatter<iGe::Event> {
    constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
    auto format(const iGe::Event& e, format_context& ctx) const { return format_to(ctx.out(), "{}", e.ToString()); }
};
