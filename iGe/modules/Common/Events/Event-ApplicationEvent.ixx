module;
#include "iGeMacro.h"

export module iGe.Event:Application;
import std;
import :Event;

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

} // namespace iGe
