module;
#include "iGeMacro.h"

export module iGe.Event:ApplicationEvent;
import :Event;

namespace iGe
{
export class IGE_API WindowResizeEvent : public Event {
public:
    WindowResizeEvent(uint32 width, uint32 height) : m_Width{width}, m_Height{height} {}

    inline uint32 GetWidth() const { return m_Width; };
    inline uint32 GetHeight() const { return m_Height; };

    inline std::string ToString() const override {
        return std::format("WindowResizeEvent: {0}, {1}", m_Width, m_Height);
    };

    inline static EventType GetStaticType() { return EventType::WindowResize; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
    inline virtual const char* GetName() const override { return "WindowResize"; }
    inline virtual uint32 GetCategoryFlags() const override { return EventCategoryApplication; }

private:
    uint32 m_Width, m_Height;
};

export class IGE_API WindowCloseEvent : public Event {
public:
    WindowCloseEvent() {}

    inline static EventType GetStaticType() { return EventType::WindowClose; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
    inline virtual const char* GetName() const override { return "WindowClose"; }
    inline virtual uint32 GetCategoryFlags() const override { return EventCategoryApplication; }
};

export class IGE_API AppTickEvent : public Event {
public:
    AppTickEvent() {}

    inline static EventType GetStaticType() { return EventType::AppTick; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
    inline virtual const char* GetName() const override { return "AppTick"; }
    inline virtual uint32 GetCategoryFlags() const override { return EventCategoryApplication; }
};

export class IGE_API AppUpdateEvent : public Event {
public:
    AppUpdateEvent() {}

    inline static EventType GetStaticType() { return EventType::AppUpdate; }
    inline virtual EventType GetEventType() const override { return GetStaticType(); }
    inline virtual const char* GetName() const override { return "AppUpdate"; }
    inline virtual uint32 GetCategoryFlags() const override { return EventCategoryApplication; }
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
