module iGe.Event;
import std;

namespace iGe
{
// ---------------------------------- Event::Implementation ----------------------------------
std::string Event::ToString() const { return GetName(); }

bool Event::IsInCategory(EventCategory category) { return GetCategoryFlags() & category; }

// ---------------------------------- EventDispatcher::Implementation ----------------------------------
EventDispatcher::EventDispatcher(Event& event) : m_Event(event) {}
} // namespace iGe