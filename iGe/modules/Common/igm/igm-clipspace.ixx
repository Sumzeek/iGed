export module iGe.igm:clipspace;
import std;
import :vec3;
import :mat4x4;

namespace iGe
{

namespace igm
{

/**
 * @brief 对给定的四阶矩阵执行平移操作。
 * @param m 需要平移的原始矩阵。
 * @param v 平移向量。
 * @return 一个新的四阶矩阵，表示原始矩阵经过平移向量变换后的结果。
 */
export template<typename T>
mat<4, 4, T> translate(mat<4, 4, T> const& m, vec<3, T> const& v);

/**
 * @brief 对给定的四阶矩阵执行旋转操作。
 * @param m 需要旋转的原始四阶矩阵。
 * @param angle 旋转角度（单位：弧度）。
 * @param v 旋转轴向量。
 * @return 一个新的四阶矩阵，表示原始四阶矩阵围绕给定的轴向量旋转指定角度后的结果。
 */
export template<typename T>
mat<4, 4, T> rotate(mat<4, 4, T> const& m, T angle, vec<3, T> const& v);

/**
 * @brief 为右手坐标系创建视图矩阵。
 * @param eye 相机的位置。
 * @param center 观察的目标点。
 * @param up 向上的方向向量。
 * @return 一个用于右手坐标系的视图矩阵。
 */
export template<typename T>
mat<4, 4, T> lookAtRH(const vec<3, T>& eye, const vec<3, T>& center, const vec<3, T>& up);

/**
 * @brief 为左手坐标系创建视图矩阵。
 * @param eye 相机的位置。
 * @param center 观察的目标点。
 * @param up 向上的方向向量。
 * @return 一个用于左手坐标系的视图矩阵。
 */
export template<typename T>
mat<4, 4, T> lookAtLH(const vec<3, T>& eye, const vec<3, T>& center, const vec<3, T>& up);

/**
 * @brief 为右手坐标系创建一个透视投影矩阵，深度范围为从0到1。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @param zFar 远裁剪平面。
 * @return 一个透视投影矩阵。
 */
export template<typename T>
mat<4, 4, T> perspectiveRH_ZO(T fovy, T aspect, T zNear, T zFar);

/**
 * @brief 为右手坐标系创建一个透视投影矩阵，深度范围为从0到1，且具有无限远裁剪平面。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @return 一个透视投影矩阵。
 */
export template<typename T>
mat<4, 4, T> perspectiveRH_ZO(T fovy, T aspect, T zNear);

/**
 * @brief 为右手坐标系创建一个透视投影矩阵，深度范围为从-1到1。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @param zFar 远裁剪平面。
 * @return 一个透视投影矩阵。
 */
export template<typename T>
mat<4, 4, T> perspectiveRH_NO(T fovy, T aspect, T zNear, T zFar);

/**
 * @brief 为右手坐标系创建一个透视投影矩阵，深度范围为从-1到1，且具有无限远裁剪平面。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @return 一个透视投影矩阵。
 */
export template<typename T>
mat<4, 4, T> perspectiveRH_NO(T fovy, T aspect, T zNear);

/**
 * @brief 为右手坐标系创建一个透视投影矩阵，深度范围为从1到0。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @param zFar 远裁剪平面。
 * @return 一个透视投影矩阵。
 */
export template<typename T>
mat<4, 4, T> perspectiveRH_OZ(T fovy, T aspect, T zNear, T zFar);

/**
 * @brief 为右手坐标系创建一个透视投影矩阵，深度范围为从1到0，且具有无限远裁剪平面。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @return 一个透视投影矩阵。
 */
export template<typename T>
mat<4, 4, T> perspectiveRH_OZ(T fovy, T aspect, T zNear);

/**
 * @brief 为左手坐标系创建一个透视投影矩阵，深度范围为从0到1。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @param zFar 远裁剪平面。
 * @return 一个透视投影矩阵。
 */
export template<typename T>
mat<4, 4, T> perspectiveLH_ZO(T fovy, T aspect, T zNear, T zFar);

/**
 * @brief 为左手坐标系创建一个透视投影矩阵，深度范围为从0到1，且具有无限远裁剪平面。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @return 一个透视投影矩阵。
 */
export template<typename T>
mat<4, 4, T> perspectiveLH_ZO(T fovy, T aspect, T zNear);

/**
 * @brief 为左手坐标系创建一个透视投影矩阵，深度范围从-1到1。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @param zFar 远裁剪平面。
 * @return 一个透视投影矩阵。
 */
export template<typename T>
mat<4, 4, T> perspectiveLH_NO(T fovy, T aspect, T zNear, T zFar);

/**
 * @brief 为左手坐标系创建一个透视投影矩阵，深度范围从-1到1，且具有无限远裁剪平面。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @return 一个透视投影矩阵。
 */
export template<typename T>
mat<4, 4, T> perspectiveLH_NO(T fovy, T aspect, T zNear);

/**
 * @brief 为左手坐标系创建一个透视投影矩阵，深度范围从1到0。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @param zFar 远裁剪平面。
 * @return 一个透视投影矩阵。
 */
export template<typename T>
mat<4, 4, T> perspectiveLH_OZ(T fovy, T aspect, T zNear, T zFar);

/**
 * @brief 为左手坐标系创建一个透视投影矩阵，深度范围从1到0，且具有无限远裁剪平面。
 * @param fovy 视野角度，在y轴方向的弧度值。
 * @param aspect 纵横比，定义为宽度除以高度。
 * @param zNear 近裁剪平面。
 * @return 一个透视投影矩阵。
 */
export template<typename T>
mat<4, 4, T> perspectiveLH_OZ(T fovy, T aspect, T zNear);

/**
 * @brief 为右手坐标系创建一个正交投影矩阵，深度范围为标准化设备坐标的0到1。
 * @param left 左侧垂直裁剪平面的坐标。
 * @param right 右侧垂直裁剪平面的坐标。
 * @param bottom 底部水平裁剪平面的坐标。
 * @param top 顶部水平裁剪平面的坐标。
 * @param zNear 近深度裁剪平面。
 * @param zFar 远深度裁剪平面。
 * @return 一个正交投影矩阵。
 */
export template<typename T>
mat<4, 4, T> orthoRH_ZO(T left, T right, T bottom, T top, T zNear, T zFar);

/**
 * @brief 为右手坐标系创建一个正交投影矩阵，深度范围为标准化设备坐标的-1到1。
 * @param left 左侧垂直裁剪平面的坐标。
 * @param right 右侧垂直裁剪平面的坐标。
 * @param bottom 底部水平裁剪平面的坐标。
 * @param top 顶部水平裁剪平面的坐标。
 * @param zNear 近深度裁剪平面。
 * @param zFar 远深度裁剪平面。
 * @return 一个正交投影矩阵。
 */
export template<typename T>
mat<4, 4, T> orthoRH_NO(T left, T right, T bottom, T top, T zNear, T zFar);

/**
 * @brief 为右手坐标系创建一个正交投影矩阵，深度范围为标准化设备坐标的1到0。
 * @param left 左侧垂直裁剪平面的坐标。
 * @param right 右侧垂直裁剪平面的坐标。
 * @param bottom 底部水平裁剪平面的坐标。
 * @param top 顶部水平裁剪平面的坐标。
 * @param zNear 近深度裁剪平面。
 * @param zFar 远深度裁剪平面。
 * @return 一个正交投影矩阵。
 */
export template<typename T>
mat<4, 4, T> orthoRH_OZ(T left, T right, T bottom, T top, T zNear, T zFar);

/**
 * @brief 为左手坐标系创建一个正交投影矩阵，深度范围为标准化设备坐标的0到1。
 * @param left 左侧垂直裁剪平面的坐标。
 * @param right 右侧垂直裁剪平面的坐标。
 * @param bottom 底部水平裁剪平面的坐标。
 * @param top 顶部水平裁剪平面的坐标。
 * @param zNear 近深度裁剪平面。
 * @param zFar 远深度裁剪平面。
 * @return 一个正交投影矩阵。
 */
export template<typename T>
mat<4, 4, T> orthoLH_ZO(T left, T right, T bottom, T top, T zNear, T zFar);

/**
 * @brief 为左手坐标系创建一个正交投影矩阵，深度范围为标准化设备坐标的-1到1。
 * @param left 左侧垂直裁剪平面的坐标。
 * @param right 右侧垂直裁剪平面的坐标。
 * @param bottom 底部水平裁剪平面的坐标。
 * @param top 顶部水平裁剪平面的坐标。
 * @param zNear 近深度裁剪平面。
 * @param zFar 远深度裁剪平面。
 * @return 一个正交投影矩阵。
 */
export template<typename T>
mat<4, 4, T> orthoLH_NO(T left, T right, T bottom, T top, T zNear, T zFar);

/**
 * @brief 为左手坐标系创建一个正交投影矩阵，深度范围为标准化设备坐标的1到0。
 * @param left 左侧垂直裁剪平面的坐标。
 * @param right 右侧垂直裁剪平面的坐标。
 * @param bottom 底部水平裁剪平面的坐标。
 * @param top 顶部水平裁剪平面的坐标。
 * @param zNear 近深度裁剪平面。
 * @param zFar 远深度裁剪平面。
 * @return 一个正交投影矩阵。
 */
export template<typename T>
mat<4, 4, T> orthoLH_OZ(T left, T right, T bottom, T top, T zNear, T zFar);

// ----------------- igm::Global -----------------

template<typename T>
mat<4, 4, T> translate(mat<4, 4, T> const& m, vec<3, T> const& v) {
    mat<4, 4, T> result(m);
    result[3] = m[0] * v[0] + m[1] * v[1] + m[2] * v[2] + m[3];
    return result;
}

template<typename T>
mat<4, 4, T> rotate(mat<4, 4, T> const& m, T angle, vec<3, T> const& v) {
    T const a = angle;
    T const c = std::cos(a);
    T const s = std::sin(a);

    vec<3, T> axis(v.normalized());
    vec<3, T> temp(axis * (1.0f - c));

    mat<4, 4, T> Rotate;
    Rotate[0][0] = c + temp[0] * axis[0];
    Rotate[0][1] = temp[0] * axis[1] + s * axis[2];
    Rotate[0][2] = temp[0] * axis[2] - s * axis[1];

    Rotate[1][0] = temp[1] * axis[0] - s * axis[2];
    Rotate[1][1] = c + temp[1] * axis[1];
    Rotate[1][2] = temp[1] * axis[2] + s * axis[0];

    Rotate[2][0] = temp[2] * axis[0] + s * axis[1];
    Rotate[2][1] = temp[2] * axis[1] - s * axis[0];
    Rotate[2][2] = c + temp[2] * axis[2];

    mat<4, 4, T> Result;
    Result[0] = m[0] * Rotate[0][0] + m[1] * Rotate[0][1] + m[2] * Rotate[0][2];
    Result[1] = m[0] * Rotate[1][0] + m[1] * Rotate[1][1] + m[2] * Rotate[1][2];
    Result[2] = m[0] * Rotate[2][0] + m[1] * Rotate[2][1] + m[2] * Rotate[2][2];
    Result[3] = m[3];
    return Result;
}

template<typename T>
mat<4, 4, T> lookAtRH(const vec<3, T>& eye, const vec<3, T>& center, const vec<3, T>& up) {
    vec<3, T> const f((center - eye).normalized());
    vec<3, T> const s((cross(f, up)).normalized());
    vec<3, T> const u(cross(s, f));

    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = s.x;
    Result[1][0] = s.y;
    Result[2][0] = s.z;
    Result[0][1] = u.x;
    Result[1][1] = u.y;
    Result[2][1] = u.z;
    Result[0][2] = -f.x;
    Result[1][2] = -f.y;
    Result[2][2] = -f.z;
    Result[3][0] = -dot(s, eye);
    Result[3][1] = -dot(u, eye);
    Result[3][2] = dot(f, eye);
    return Result;
}

template<typename T>
mat<4, 4, T> lookAtLH(const vec<3, T>& eye, const vec<3, T>& center, const vec<3, T>& up) {
    vec<3, T> const f((center - eye).normalized());
    vec<3, T> const s((cross(f, up)).normalized());
    vec<3, T> const u(cross(f, s));

    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = s.x;
    Result[1][0] = s.y;
    Result[2][0] = s.z;
    Result[0][1] = u.x;
    Result[1][1] = u.y;
    Result[2][1] = u.z;
    Result[0][2] = -f.x;
    Result[1][2] = -f.y;
    Result[2][2] = -f.z;
    Result[3][0] = -dot(s, eye);
    Result[3][1] = -dot(u, eye);
    Result[3][2] = dot(f, eye);
    return Result;
}

template<typename T>
// depth range: 0.0(near plane) -> 1.0(far plane)
mat<4, 4, T> perspectiveRH_ZO(T fovy, T aspect, T zNear, T zFar) {
    assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = zFar / (zNear - zFar);
    Result[2][3] = -static_cast<T>(1);
    Result[3][2] = -(zFar * zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
// depth range: 0.0(near plane) -> 1.0(far plane)
mat<4, 4, T> perspectiveRH_ZO(T fovy, T aspect, T zNear) {
    assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = -static_cast<T>(1);
    Result[2][3] = -static_cast<T>(1);
    Result[3][2] = -zNear;
    return Result;
}

template<typename T>
// depth range: -1.0(near plane) -> 1.0(far plane)
mat<4, 4, T> perspectiveRH_NO(T fovy, T aspect, T zNear, T zFar) {
    assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = -(zFar + zNear) / (zFar - zNear);
    Result[2][3] = -static_cast<T>(1);
    Result[3][2] = -(static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
// depth range: -1.0(near plane) -> 1.0(far plane)
mat<4, 4, T> perspectiveRH_NO(T fovy, T aspect, T zNear) {
    assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = -static_cast<T>(1);
    Result[2][3] = -static_cast<T>(1);
    Result[3][2] = -(static_cast<T>(2) * zNear);
    return Result;
}

template<typename T>
// depth range: 1.0(near plane) -> 0.0(far plane)
mat<4, 4, T> perspectiveRH_OZ(T fovy, T aspect, T zNear, T zFar) {
    assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = std::tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = zNear / (zFar - zNear);
    Result[2][3] = -static_cast<T>(1);
    Result[3][2] = (zFar * zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
// depth range: 1.0(near plane) -> 0.0(far plane)
mat<4, 4, T> perspectiveRH_OZ(T fovy, T aspect, T zNear) {
    assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = std::tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][3] = -static_cast<T>(1);
    Result[3][2] = zNear;
    return Result;
}

template<typename T>
mat<4, 4, T> perspectiveLH_ZO(T fovy, T aspect, T zNear, T zFar) {
    assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = zFar / (zFar - zNear);
    Result[2][3] = static_cast<T>(1);
    Result[3][2] = -(zFar * zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
mat<4, 4, T> perspectiveLH_ZO(T fovy, T aspect, T zNear) {
    assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = static_cast<T>(1);
    Result[2][3] = static_cast<T>(1);
    Result[3][2] = -zNear;
    return Result;
}

template<typename T>
mat<4, 4, T> perspectiveLH_NO(T fovy, T aspect, T zNear, T zFar) {
    assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = (zFar + zNear) / (zFar - zNear);
    Result[2][3] = static_cast<T>(1);
    Result[3][2] = -(static_cast<T>(2) * zFar * zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
mat<4, 4, T> perspectiveLH_NO(T fovy, T aspect, T zNear) {
    assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = static_cast<T>(1);
    Result[2][3] = static_cast<T>(1);
    Result[3][2] = -(static_cast<T>(2) * zNear);
    return Result;
}

template<typename T>
mat<4, 4, T> perspectiveLH_OZ(T fovy, T aspect, T zNear, T zFar) {
    assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][2] = -(zNear) / (zFar - zNear);
    Result[2][3] = static_cast<T>(1);
    Result[3][2] = (zFar * zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
mat<4, 4, T> perspectiveLH_OZ(T fovy, T aspect, T zNear) {
    assert(std::abs(aspect - std::numeric_limits<T>::epsilon()) > static_cast<T>(0));

    T const tanHalfFovy = tan(fovy / static_cast<T>(2));

    mat<4, 4, T> Result(static_cast<T>(0));
    Result[0][0] = static_cast<T>(1) / (aspect * tanHalfFovy);
    Result[1][1] = static_cast<T>(1) / (tanHalfFovy);
    Result[2][3] = static_cast<T>(1);
    Result[3][2] = zNear;
    return Result;
}

template<typename T>
// depth range: 0.0(near plane) -> 1.0(far plane)
mat<4, 4, T> orthoRH_ZO(T left, T right, T bottom, T top, T zNear, T zFar) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = -static_cast<T>(1) / (zFar - zNear);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = -zNear / (zFar - zNear);
    return Result;
}

template<typename T>
// depth range: -1.0(near plane) -> 1.0(far plane)
mat<4, 4, T> orthoRH_NO(T left, T right, T bottom, T top, T zNear, T zFar) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = -static_cast<T>(2) / (zFar - zNear);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = -(zFar + zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
// depth range: 1.0(near plane) -> 0.0(far plane)
mat<4, 4, T> orthoRH_OZ(T left, T right, T bottom, T top, T zNear, T zFar) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = static_cast<T>(1) / (zFar - zNear);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = zFar / (zFar - zNear);
    return Result;
}

template<typename T>
mat<4, 4, T> orthoLH_ZO(T left, T right, T bottom, T top, T zNear, T zFar) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = static_cast<T>(1) / (zFar - zNear);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = -zNear / (zFar - zNear);
    return Result;
}

template<typename T>
mat<4, 4, T> orthoLH_NO(T left, T right, T bottom, T top, T zNear, T zFar) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = static_cast<T>(2) / (zFar - zNear);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = -(zFar + zNear) / (zFar - zNear);
    return Result;
}

template<typename T>
mat<4, 4, T> orthoLH_OZ(T left, T right, T bottom, T top, T zNear, T zFar) {
    mat<4, 4, T> Result(static_cast<T>(1));
    Result[0][0] = static_cast<T>(2) / (right - left);
    Result[1][1] = static_cast<T>(2) / (top - bottom);
    Result[2][2] = -static_cast<T>(1) / (zFar - zNear);
    Result[3][0] = -(right + left) / (right - left);
    Result[3][1] = -(top + bottom) / (top - bottom);
    Result[3][2] = zFar / (zFar - zNear);
    return Result;
}

} // namespace igm

} // namespace iGe
