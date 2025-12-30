export module iGe.Flags;
import std;

namespace iGe
{

export template<typename Enum>
struct Flags {
public:
    using Underlying = std::underlying_type_t<Enum>;

    Flags() = default;
    Flags(Enum e) : Value(static_cast<Underlying>(e)) {}
    explicit Flags(Underlying value) : Value(value) {}

    Underlying GetValue() const { return Value; }

    constexpr bool operator==(Flags rhs) const { return Value == rhs.Value; }

    constexpr bool operator!=(Flags rhs) const { return Value != rhs.Value; }

    bool HasFlag(Enum e) const { return (Value & static_cast<Underlying>(e)) != 0; }

    void AddFlag(Enum e) { Value |= static_cast<Underlying>(e); }

    void Reset() { Value = 0; }

private:
    Underlying Value = 0;
};

export template<typename Enum>
constexpr Flags<Enum> operator|(Enum lhs, Enum rhs) {
    using U = std::underlying_type_t<Enum>;
    return Flags<Enum>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

export template<typename Enum>
constexpr Flags<Enum> operator|(Flags<Enum> lhs, Enum rhs) {
    lhs.AddFlag(rhs);
    return lhs;
}

export template<typename Enum>
constexpr Flags<Enum> operator|(Flags<Enum> lhs, Flags<Enum> rhs) {
    return Flags<Enum>(lhs.ToUnderlying() | rhs.ToUnderlying());
}

export template<typename Enum>
constexpr Flags<Enum>& operator|=(Flags<Enum>& lhs, Enum rhs) {
    lhs.AddFlag(rhs);
    return lhs;
}

export template<typename Enum>
constexpr Flags<Enum>& operator|=(Flags<Enum>& lhs, Flags<Enum> rhs) {
    lhs = lhs | rhs;
    return lhs;
}

} // namespace iGe
