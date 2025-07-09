module;
#include "iGeMacro.h"

module iGe.Event;
import :ApplicationEvent;

import std;

namespace iGe
{

/////////////////////////////////////////////////////////////////////////////
// WindowResizeEvent ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

WindowResizeEvent::WindowResizeEvent(unsigned int width, unsigned int height) : m_Width{width}, m_Height{height} {}

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

/////////////////////////////////////////////////////////////////////////////
// WindowCloseEvent /////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

WindowCloseEvent::WindowCloseEvent() {}

EventType WindowCloseEvent::GetStaticType() { return EventType::WindowClose; }

EventType WindowCloseEvent::GetEventType() const { return GetStaticType(); }

const char* WindowCloseEvent::GetName() const { return "WindowClose"; }

int WindowCloseEvent::GetCategoryFlags() const { return EventCategoryApplication; }

/////////////////////////////////////////////////////////////////////////////
// AppTickEvent /////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

AppTickEvent::AppTickEvent() {}

EventType AppTickEvent::GetStaticType() { return EventType::AppTick; }

EventType AppTickEvent::GetEventType() const { return GetStaticType(); }

const char* AppTickEvent::GetName() const { return "AppTick"; }

int AppTickEvent::GetCategoryFlags() const { return EventCategoryApplication; }

/////////////////////////////////////////////////////////////////////////////
// AppUpdateEvent ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

AppUpdateEvent::AppUpdateEvent() {}

EventType AppUpdateEvent::GetStaticType() { return EventType::AppUpdate; }

EventType AppUpdateEvent::GetEventType() const { return GetStaticType(); }

const char* AppUpdateEvent::GetName() const { return "AppUpdate"; }

int AppUpdateEvent::GetCategoryFlags() const { return EventCategoryApplication; }

/////////////////////////////////////////////////////////////////////////////
// AppRenderEvent ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

AppRenderEvent::AppRenderEvent() {}

EventType AppRenderEvent::GetStaticType() { return EventType::AppRender; }

EventType AppRenderEvent::GetEventType() const { return GetStaticType(); }

const char* AppRenderEvent::GetName() const { return "AppRender"; }

int AppRenderEvent::GetCategoryFlags() const { return EventCategoryApplication; }
} // namespace iGe