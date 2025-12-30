module;
#include "iGeMacro.h"

export module iGe.SmartPointer;
import iGe.Types;

namespace iGe
{

export template<typename T>
class IGE_API Scope : public std::unique_ptr<T> {
public:
    using std::unique_ptr<T>::unique_ptr;

    explicit Scope(const std::unique_ptr<T>& ptr) : std::unique_ptr<T>(ptr) {}
    explicit Scope(std::unique_ptr<T>&& ptr) : std::unique_ptr<T>(std::move(ptr)) {}

    T* Get() const noexcept { return this->get(); }
};

export template<typename T, typename... Args>
constexpr Scope<T> CreateScope(Args&&... args) {
    return Scope<T>(std::make_unique<T>(std::forward<Args>(args)...));
}

export template<typename T>
class IGE_API Ref : public std::shared_ptr<T> {
public:
    using std::shared_ptr<T>::shared_ptr;

    explicit Ref(const std::shared_ptr<T>& ptr) : std::shared_ptr<T>(ptr) {}
    explicit Ref(std::shared_ptr<T>&& ptr) : std::shared_ptr<T>(std::move(ptr)) {}

    Ref& operator=(const std::shared_ptr<T>& ptr) {
        std::shared_ptr<T>::operator=(ptr);
        return *this;
    }

    Ref& operator=(std::shared_ptr<T>&& ptr) {
        std::shared_ptr<T>::operator=(std::move(ptr));
        return *this;
    }

    T* Get() const noexcept { return this->get(); }
};

export template<typename T, typename... Args>
constexpr Ref<T> CreateRef(Args&&... args) {
    return Ref<T>(std::make_shared<T>(std::forward<Args>(args)...));
}

} // namespace iGe
