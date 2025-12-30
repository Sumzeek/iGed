module;
#include "iGeMacro.h"

export module iGe.Event:ApplicationEvent;
import :Event;

namespace iGe
{

export class IGE_API WindowResizeEvent : public Event {
public:
    WindowResizeEvent(uint32 width, uint32 height) : m_Width{width}, m_Height{height} {}

    uint32 GetWidth() const { return m_Width; };
    uint32 GetHeight() const { return m_Height; };

    string ToString() const override { return std::format("WindowResizeEvent: {0}, {1}", m_Width, m_Height); };

    static EventType GetStaticType() { return EventType::WindowResize; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "WindowResize"; }
    virtual uint32 GetCategoryFlags() const override { return EventCategoryApplication; }

private:
    uint32 m_Width, m_Height;
};

export class IGE_API WindowCloseEvent : public Event {
public:
    WindowCloseEvent() {}

    static EventType GetStaticType() { return EventType::WindowClose; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "WindowClose"; }
    virtual uint32 GetCategoryFlags() const override { return EventCategoryApplication; }
};

export class IGE_API AppTickEvent : public Event {
public:
    AppTickEvent() {}

    static EventType GetStaticType() { return EventType::AppTick; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "AppTick"; }
    virtual uint32 GetCategoryFlags() const override { return EventCategoryApplication; }
};

export class IGE_API AppUpdateEvent : public Event {
public:
    AppUpdateEvent() {}

    static EventType GetStaticType() { return EventType::AppUpdate; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "AppUpdate"; }
    virtual uint32 GetCategoryFlags() const override { return EventCategoryApplication; }
};

export class IGE_API AppRenderEvent : public Event {
public:
    AppRenderEvent() {}

    static EventType GetStaticType() { return EventType::AppRender; }
    virtual EventType GetEventType() const override { return GetStaticType(); }
    virtual const char* GetName() const override { return "AppRender"; }
    virtual uint32 GetCategoryFlags() const override { return EventCategoryApplication; }
};

} // namespace iGe
