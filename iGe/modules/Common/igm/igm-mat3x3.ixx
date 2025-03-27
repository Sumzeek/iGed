module;
#include <assert.h>

export module iGe.igm:mat3x3;
import :common;

namespace iGe
{

namespace igm
{

export typedef mat<3, 3, float> mat3;

export template<typename T>
class mat<3, 3, T> {
public:
    /**
     * @brief 默认构造函数。将矩阵初始化为单位矩阵。
     */
    mat() : value{} { setToIdentity(); }

    /**
     * @brief 构造函数，使用指定的值初始化矩阵。
     * @param x0 第一列，第一行的值。
     * @param y0 第一列，第二行的值。
     * @param z0 第一列，第三行的值。
     * @param x1 第二列，第一行的值。
     * @param y1 第二列，第二行的值。
     * @param z1 第二列，第三行的值。
     * @param x2 第三列，第一行的值。
     * @param y2 第三列，第二行的值。
     * @param z2 第三列，第三行的值。
     */
    mat(const T& x0, const T& y0, const T& z0, const T& x1, const T& y1, const T& z1, const T& x2, const T& y2,
        const T& z2)
        : value{vec<3, T>(x0, y0, z0), vec<3, T>(x1, y1, z1), vec<3, T>(x2, y2, z2)} {}

    /**
     * @brief 构造函数，使用给定的列向量初始化矩阵。
     * @param v0 第一个列向量。
     * @param v1 第二个列向量。
     * @param v2 第三个列向量。
     */
    mat(const vec<3, T>& v0, const vec<3, T>& v1, const vec<3, T>& v2) : value{} {
        this->value[0] = v0;
        this->value[1] = v1;
        this->value[2] = v2;
    }

    /**
     * @brief 构造函数，将矩阵初始化为对角矩阵。
     * @param diagonal 设置对角线上的值。
     */
    explicit mat(const T& diagonal) : value{} {
        this->value[0] = vec<3, T>(diagonal, 0, 0);
        this->value[1] = vec<3, T>(0, diagonal, 0);
        this->value[2] = vec<3, T>(0, 0, diagonal);
    }

    /**
     * @brief 拷贝构造函数。
     * @param other 要拷贝的矩阵。
     */
    mat(const mat<3, 3, T>& other) : value{} {
        this->value[0] = other.value[0];
        this->value[1] = other.value[1];
        this->value[2] = other.value[2];
    }

    /**
     * @brief 三阶矩阵流插入运算符。
     * @param os 输出流。
     * @param m 要输出的三阶矩阵对象。
     * @return 输出流的引用。
     */
    friend std::ostream& operator<<(std::ostream& os, const mat<3, 3, T>& m) {
        os << "[" << m[0][0] << ", " << m[1][0] << ", " << m[2][0] << "\n";
        os << " " << m[0][1] << ", " << m[1][1] << ", " << m[2][1] << "\n";
        os << " " << m[0][2] << ", " << m[1][2] << ", " << m[2][2] << "]";
        return os;
    }

    /**
     * @brief 赋值运算符。
     * @param other 要赋值的三阶矩阵对象。
     * @return 经过赋值后，本对象的引用。
     */
    mat<3, 3, T>& operator=(const mat<3, 3, T>& other) {
        if (this != &other) {
            this->value[0] = other[0];
            this->value[1] = other[1];
            this->value[2] = other[2];
        }
        return *this;
    }

    /**
     * @brief  三阶矩阵的加法运算符。
     * @param  other  要加的三阶矩阵对象。
     * @return  一个新的三阶矩阵，其为两个三阶矩阵的加法结果。
     */
    mat<3, 3, T> operator+(const mat<3, 3, T>& other) const {
        mat<3, 3, T> result;
        result[0] = this->value[0] + other[0];
        result[1] = this->value[1] + other[1];
        result[2] = this->value[2] + other[2];
        return result;
    }

    /**
     * @brief  三阶矩阵的加法赋值运算符。
     * @param  other  要加上的三阶矩阵对象。
     * @return  经过加法后的本对象引用。
     */
    mat<3, 3, T>& operator+=(const mat<3, 3, T>& other) {
        this->value[0] += other[0];
        this->value[1] += other[1];
        this->value[2] += other[2];
        return *this;
    }

    /**
     * @brief  三阶矩阵的减法运算符。
     * @param  other  要减去的三阶矩阵对象。
     * @return  一个新的三阶矩阵，其为两个三阶矩阵的减法结果。
     */
    mat<3, 3, T> operator-(const mat<3, 3, T>& other) const {
        mat<3, 3, T> result;
        result[0] = this->value[0] - other[0];
        result[1] = this->value[1] - other[1];
        result[2] = this->value[2] - other[2];
        return result;
    }

    /**
     * @brief  三阶矩阵的减法赋值运算符。
     * @param  other  要减去的三阶矩阵对象。
     * @return  经过减法后的本对象引用。
     */
    mat<3, 3, T>& operator-=(const mat<3, 3, T>& other) {
        this->value[0] -= other[0];
        this->value[1] -= other[1];
        this->value[2] -= other[2];
        return *this;
    }

    /**
     * @brief  三阶矩阵的常数乘法运算符。
     * @param  scalar  要乘的常数值。
     * @return  一个新的三阶矩阵，其为三阶矩阵乘以常数的结果。
     */
    mat<3, 3, T> operator*(T scalar) const {
        mat<3, 3, T> result;
        result[0] = this->value[0] * scalar;
        result[1] = this->value[0] * scalar;
        result[2] = this->value[0] * scalar;
        return result;
    }

    /**
     * @brief  三阶矩阵的常数乘法赋值运算符。
     * @param  scalar  要乘的常数值。
     * @return  经过常数乘法后的本对象引用。
     */
    mat<3, 3, T>& operator*=(const T& scalar) {
        this->value[0] *= scalar;
        this->value[1] *= scalar;
        this->value[2] *= scalar;
        return *this;
    }

    /**
     * @brief 三阶矩阵和三维向量的矩阵-向量乘法运算符。
     *
     * 该运算符将一个三阶矩阵与一个三维向量相乘。结果向量的计算如下：
     *
     * result[0] = (m[0][0] * v[0]) + (m[1][0] * v[1]) + (m[2][0] * v[2])
     * result[1] = (m[0][1] * v[0]) + (m[1][1] * v[1]) + (m[2][1] * v[2])
     * result[2] = (m[0][2] * v[0]) + (m[1][2] * v[1]) + (m[2][2] * v[2])
     *
     * 其中，m 是矩阵，v 是向量。
     *
     * @param v 要相乘的三维向量对象。
     * @return  经过乘法运算后的新三维向量。
     */
    vec<3, T> operator*(const vec<3, T>& v) const {
        return vec<3, T>(value[0][0] * v[0] + value[1][0] * v[1] + value[2][0] * v[2],
                         value[0][1] * v[0] + value[1][1] * v[1] + value[2][1] * v[2],
                         value[0][2] * v[0] + value[1][2] * v[1] + value[2][2] * v[2]);
    }


    /**
     * @brief 三阶矩阵和三阶矩阵的矩阵乘法运算符。
     * @param other 要相乘的三阶矩阵对象。
     * @return 经过乘法运算后的新的三阶矩阵。
     */
    mat<3, 3, T> operator*(const mat<3, 3, T>& other) const {
        mat<3, 3, T> result;
        for (int c = 0; c < 3; ++c) {
            for (int r = 0; r < 3; ++r) {
                result.value[c][r] = value[0][r] * other.value[c][0] + value[1][r] * other.value[c][1] +
                                     value[2][r] * other.value[c][2];
            }
        }
        return result;
    }

    /**
     * @brief 三阶矩阵和三阶矩阵的矩阵乘法赋值运算符。
     * @param other 要相乘的三阶矩阵对象。
     * @return 经过矩阵乘法后的本对象引用。
     */
    mat<3, 3, T>& operator*=(const mat<3, 3, T>& other) {
        mat<3, 3, T> result;
        for (int c = 0; c < 3; ++c) {
            for (int r = 0; r < 3; ++r) {
                result.value[c][r] = value[0][r] * other.value[c][0] + value[1][r] * other.value[c][1] +
                                     value[2][r] * other.value[c][2];
            }
        }
        *this = result;
        return *this;
    }

    /**
     * @brief 下标运算符，用于访问矩阵的列向量。
     * @param index 列的索引（0-2）。
     * @return 返回列向量的引用。
     */
    vec<3, T>& operator[](int index) {
        assert(index >= 0 && index < 3);
        return value[index];
    }

    /**
     * @brief 下标运算符，用于访问矩阵的列向量（常数版本）。
     * @param index 列的索引（0-2）。
     * @return 返回列向量的常量引用。
     */
    const vec<3, T>& operator[](int index) const {
        assert(index >= 0 && index < 3);
        return value[index];
    }

    /**
     * @brief 获取指向矩阵底层数据的指针。
     * @return 指向矩阵第一个列向量元素的指针。
     */
    T const* data() const { return &value[0].x; }

    /**
     * @brief 将矩阵设置为单位矩阵。
     */
    void setToIdentity();

    /**
     * @brief 计算矩阵的行列式。
     * @return 行列式值。
     */
    [[nodiscard]] T determinant() const;

    /**
     * @brief 计算矩阵的伴随矩阵（伴随矩阵）。
     * @return 一个新的三阶矩阵，其为调用对象的伴随矩阵。
     */
    [[nodiscard]] mat<3, 3, T> adjoint() const;

    /**
     * @brief 计算矩阵的逆矩阵。
     * @return 一个新的三阶矩阵，其为调用对象的逆矩阵。
     */
    [[nodiscard]] mat<3, 3, T> invert() const;

    /**
     * @brief 计算矩阵的转置矩阵。
     * @return 一个新的三阶矩阵，其为调用对象的转置矩阵。
     */
    [[nodiscard]] mat<3, 3, T> transpose() const;

protected:
    vec<3, T> value[3]; /**< 用于保存矩阵的列向量的数组。 */
};

// ----------------- mat<3, 3, T>::Implementation -----------------

template<typename T>
void mat<3, 3, T>::setToIdentity() {
    value[0][0] = 1.0f;
    value[0][1] = 0.0f;
    value[0][2] = 0.0f;
    value[1][0] = 0.0f;
    value[1][1] = 1.0f;
    value[1][2] = 0.0f;
    value[2][0] = 0.0f;
    value[2][1] = 0.0f;
    value[2][2] = 1.0f;
}

template<typename T>
T mat<3, 3, T>::determinant() const {
    T a1, a2, a3, b1, b2, b3, c1, c2, c3;

    a1 = this->value[0][0];
    a2 = this->value[0][1];
    a3 = this->value[0][2];

    b1 = this->value[1][0];
    b2 = this->value[1][1];
    b3 = this->value[1][2];

    c1 = this->value[2][0];
    c2 = this->value[2][1];
    c3 = this->value[2][2];

    return a1 * determinant2x2(b2, b3, c2, c3) - b1 * determinant2x2(a2, a3, c2, c3) +
           c1 * determinant2x2(a2, a3, b2, b3);
}

template<typename T>
mat<3, 3, T> mat<3, 3, T>::adjoint() const {
    mat<3, 3, T> adj;
    T a1, a2, a3, a4, b1, b2, b3, b4, c1, c2, c3, c4, d1, d2, d3, d4;

    a1 = this->value[0][0];
    a2 = this->value[0][1];
    a3 = this->value[0][2];

    b1 = this->value[1][0];
    b2 = this->value[1][1];
    b3 = this->value[1][2];

    c1 = this->value[2][0];
    c2 = this->value[2][1];
    c3 = this->value[2][2];

    adj[0][0] = determinant2x2(b2, b3, c2, c3);
    adj[0][1] = -determinant2x2(a2, a3, c2, c3);
    adj[0][2] = determinant2x2(a2, a3, b2, b3);

    adj[1][0] = -determinant2x2(b1, b3, c1, c3);
    adj[1][1] = determinant2x2(a1, a3, c1, c3);
    adj[1][2] = -determinant2x2(a1, a3, b1, b3);

    adj[2][0] = determinant2x2(b1, b2, c1, c2);
    adj[2][1] = -determinant2x2(a1, a2, c1, c2);
    adj[2][2] = determinant2x2(a1, a2, b1, b2);

    return adj;
}

template<typename T>
mat<3, 3, T> mat<3, 3, T>::invert() const {
    T det = determinant();
    if (det == 0) {
        std::cout << "igm Error: mat<3, 3, T>-matrix is not invertible" << std::endl;
        throw std::runtime_error("igm Error: mat<3, 3, T>-matrix is not invertible");
    }

    mat<3, 3, T> adj = adjoint();

    mat<3, 3, T> inv;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) inv[i][j] = adj[i][j] / det;
    return inv;
}

template<typename T>
mat<3, 3, T> mat<3, 3, T>::transpose() const {
    mat<3, 3, T> trans;
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) { trans.value[i][j] = value[j][i]; }
    }
    return trans;
}

// ----------------- igm::Global -----------------

/**
 * @brief 获取三阶矩阵矩阵数据的常量指针。
 * @param m 三阶矩阵对象。
 * @return 矩阵 m 底层数据的常量指针。
 */
export template<typename T>
T const* value_ptr(const mat<3, 3, T>& m) {
    return m.data();
}

/**
 * @brief 计算三阶矩阵的行列式。
 * @param m 三阶矩阵对象。
 * @return 矩阵 m 的行列式值。
 */
export template<typename T>
T determinant(const mat<3, 3, T>& m) {
    return m.determinant();
}

/**
 * @brief 把一个三阶矩阵设置为其伴随矩阵。
 * @param m 三阶矩阵对象。
 * @return 矩阵 m 的伴随矩阵。
 */
export template<typename T>
mat<3, 3, T> adjoint(mat<3, 3, T>& m) {
    mat<3, 3, T> adj = m.adjoint();
    m = adj;
    return adj;
}

/**
 * @brief 把一个三阶矩阵设置为其逆矩阵
 * @param m 三阶矩阵对象。
 * @return 矩阵 m 的逆矩阵。
 */
export template<typename T>
mat<3, 3, T> invert(mat<3, 3, T>& m) {
    mat<3, 3, T> inv = m.invert();
    m = inv;
    return inv;
}

/**
 * @brief 把一个三阶矩阵设置为其转置矩阵
 * @param m 三阶矩阵对象。
 * @return 矩阵 m 的转置矩阵。
 */
export template<typename T>
mat<3, 3, T> transpose(mat<3, 3, T>& m) {
    mat<3, 3, T> trans = m.transpose();
    m = trans;
    return trans;
}

} // namespace igm

} // namespace iGe
