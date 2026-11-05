#pragma once

#include <cmath>

namespace jcad::geom
{

  /** Free 2D vector (direction and magnitude). */
  template <typename T>
  struct Vec2
  {
    T x{};
    T y{};

    constexpr Vec2 operator+(const Vec2& other) const { return { x + other.x, y + other.y }; }
    constexpr Vec2 operator-(const Vec2& other) const { return { x - other.x, y - other.y }; }
    constexpr Vec2 operator-() const { return { -x, -y }; }
    constexpr Vec2 operator*(const T& scale) const { return { x * scale, y * scale }; }
    constexpr Vec2 operator/(const T& scale) const { return { x / scale, y / scale }; }

    constexpr Vec2& operator+=(const Vec2& other)
    {
      x += other.x;
      y += other.y;
      return *this;
    }

    constexpr Vec2& operator-=(const Vec2& other)
    {
      x -= other.x;
      y -= other.y;
      return *this;
    }

    constexpr Vec2& operator*=(const T& scale)
    {
      x *= scale;
      y *= scale;
      return *this;
    }

    constexpr bool operator==(const Vec2&) const = default;
  };

  template <typename T>
  constexpr Vec2<T> operator*(const T& scale, const Vec2<T>& v)
  {
    return v * scale;
  }

  template <typename T>
  [[nodiscard]] constexpr T Dot(const Vec2<T>& a, const Vec2<T>& b)
  {
    return a.x * b.x + a.y * b.y;
  }

  /** z component of the cross product: the signed area of the parallelogram (a, b). */
  template <typename T>
  [[nodiscard]] constexpr T Cross(const Vec2<T>& a, const Vec2<T>& b)
  {
    return a.x * b.y - a.y * b.x;
  }

  template <typename T>
  [[nodiscard]] constexpr T SquaredLength(const Vec2<T>& v)
  {
    return Dot(v, v);
  }

  /** Only for scalars that have a square root (not for exact rationals). */
  template <typename T>
  [[nodiscard]] T Length(const Vec2<T>& v)
  {
    using std::sqrt;
    return sqrt(SquaredLength(v));
  }

  /** The vector rotated by +90 degrees. */
  template <typename T>
  [[nodiscard]] constexpr Vec2<T> Perp(const Vec2<T>& v)
  {
    return { -v.y, v.x };
  }

} // namespace jcad::geom
