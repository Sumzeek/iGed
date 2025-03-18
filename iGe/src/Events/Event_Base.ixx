#include "Common/Core.h"
#include "Common/iGepch.h"

export module iGe.Event:Base;

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

    inline bool IsInCategory(EventCategory category) { return GetCategoryFlags() & category; }
};

std::string Event::ToString() const { return GetName(); }

export class IGE_API EventDispatcher {
public:
    EventDispatcher(Event& event);

    template<class T>
    bool Dispatch(std::function<bool(T&)> func);

private:
    Event& m_Event;
};

EventDispatcher::EventDispatcher(Event& event) : m_Event(event) {}
template<class T>
bool EventDispatcher::Dispatch(std::function<bool(T&)> func) {
    if (m_Event.GetEventType() == T::GetStaticType()) {
        m_Event.m_Handled = func(*(T*) &m_Event);
        return true;
    }
    return false;
}

export inline std::ostream& operator<<(std::ostream& os, const Event& e) { return os << e.ToString(); }
export inline std::string format_as(const Event& e) { return e.ToString(); }

} // namespace iGe
