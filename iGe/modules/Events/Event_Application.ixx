module;
#include "Common/Core.h"
#include "Common/iGepch.h"

export module iGe.Event:Application;

import :Base;

namespace iGe
{

export class IGE_API WindowResizeEvent : public Event {
public:
    WindowResizeEvent(unsigned int width, unsigned int height);

    inline unsigned int GetWidth() const;
    inline unsigned int GetHeight() const;

    std::string ToString() const override;

    //EVENT_CLASS_TYPE(WindowResize)
    static EventType GetStaticType();
    virtual EventType GetEventType() const;
    virtual const char* GetName() const;

    //EVENT_CLASS_CATEGORY(EventCategoryApplication)
    virtual int GetCategoryFlags() const override;

private:
    unsigned int m_Width, m_Height;
};

export class IGE_API WindowCloseEvent : public Event {
public:
    WindowCloseEvent();

    //EVENT_CLASS_TYPE(WindowClose)
    static EventType GetStaticType();
    virtual EventType GetEventType() const override;
    virtual const char* GetName() const override;

    //EVENT_CLASS_CATEGORY(EventCategoryApplication)
    virtual int GetCategoryFlags() const override;
};

export class IGE_API AppTickEvent : public Event {
public:
    AppTickEvent();

    //EVENT_CLASS_TYPE(AppTick)
    static EventType GetStaticType();
    virtual EventType GetEventType() const override;
    virtual const char* GetName() const override;

    //EVENT_CLASS_CATEGORY(EventCategoryApplication)
    virtual int GetCategoryFlags() const override;
};

export class IGE_API AppUpdateEvent : public Event {
public:
    AppUpdateEvent();

    //EVENT_CLASS_TYPE(AppUpdate)
    static EventType GetStaticType();
    virtual EventType GetEventType() const override;
    virtual const char* GetName() const override;

    //EVENT_CLASS_CATEGORY(EventCategoryApplication)
    virtual int GetCategoryFlags() const override;
};

export class IGE_API AppRenderEvent : public Event {
public:
    AppRenderEvent();

    //EVENT_CLASS_TYPE(AppRender)
    static EventType GetStaticType();
    virtual EventType GetEventType() const override;
    virtual const char* GetName() const override;

    //EVENT_CLASS_CATEGORY(EventCategoryApplication)
    virtual int GetCategoryFlags() const override;
};

// ----------------- WindowResizeEvent::Implementation -----------------
WindowResizeEvent::WindowResizeEvent(unsigned int width, unsigned int height) : m_Width(width), m_Height(height) {}

unsigned int WindowResizeEvent::GetWidth() const { return m_Width; }
unsigned int WindowResizeEvent::GetHeight() const { return m_Height; }

std::string WindowResizeEvent::ToString() const {
    return std::format("WindowResizeEvent: {0}, {1}", m_Width, m_Height);
}

//EVENT_CLASS_TYPE(WindowResize)
EventType WindowResizeEvent::GetStaticType() { return EventType::WindowResize; }
EventType WindowResizeEvent::GetEventType() const { return GetStaticType(); }
const char* WindowResizeEvent::GetName() const { return "WindowResize"; }

//EVENT_CLASS_CATEGORY(EventCategoryApplication)
int WindowResizeEvent::GetCategoryFlags() const { return EventCategoryApplication; }

// ----------------- WindowCloseEvent::Implementation -----------------
WindowCloseEvent::WindowCloseEvent() {}

EventType WindowCloseEvent::GetStaticType() { return EventType::WindowClose; }
EventType WindowCloseEvent::GetEventType() const { return GetStaticType(); }
const char* WindowCloseEvent::GetName() const { return "WindowClose"; }

int WindowCloseEvent::GetCategoryFlags() const { return EventCategoryApplication; }

// ----------------- AppTickEvent::Implementation -----------------
AppTickEvent::AppTickEvent() {}

EventType AppTickEvent::GetStaticType() { return EventType::AppTick; }
EventType AppTickEvent::GetEventType() const { return GetStaticType(); }
const char* AppTickEvent::GetName() const { return "AppTick"; }

int AppTickEvent::GetCategoryFlags() const { return EventCategoryApplication; }

// ----------------- AppUpdateEvent::Implementation -----------------
AppUpdateEvent::AppUpdateEvent() {}

EventType AppUpdateEvent::GetStaticType() { return EventType::AppUpdate; }
EventType AppUpdateEvent::GetEventType() const { return GetStaticType(); }
const char* AppUpdateEvent::GetName() const { return "AppUpdate"; }

int AppUpdateEvent::GetCategoryFlags() const { return EventCategoryApplication; }

// ----------------- AppRenderEvent::Implementation -----------------
AppRenderEvent::AppRenderEvent() {}

EventType AppRenderEvent::GetStaticType() { return EventType::AppRender; }
EventType AppRenderEvent::GetEventType() const { return GetStaticType(); }
const char* AppRenderEvent::GetName() const { return "AppRender"; }

int AppRenderEvent::GetCategoryFlags() const { return EventCategoryApplication; }

} // namespace iGe
