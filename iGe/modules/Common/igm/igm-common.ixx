export module iGe.igm:common;
import std;

namespace iGe
{

export constexpr double IGE_PI = 3.14159265358979323846;

namespace igm
{

export template<int N, typename T>
class vec {};

export template<int M, int N, typename T>
class mat {};

export template<typename T>
double radians(T degrees) {
    static_assert(std::numeric_limits<T>::is_iec559, "'radians' only accept Ting-point input");
    return static_cast<double>(degrees) * static_cast<double>(0.01745329251994329576923690768489);
}

export template<typename T>
double determinant2x2(T a, T b, T c, T d) {
    return static_cast<double>(a) * static_cast<double>(d) - static_cast<double>(b) * static_cast<double>(c);
}

export template<typename T>
double determinant3x3(T a1, T a2, T a3, T b1, T b2, T b3, T c1, T c2, T c3) {
    return (static_cast<double>(a1) * determinant2x2(b2, b3, c2, c3) -
            static_cast<double>(b1) * determinant2x2(a2, a3, c2, c3) +
            static_cast<double>(c1) * determinant2x2(a2, a3, b2, b3));
}

} // namespace igm

} // namespace iGe
