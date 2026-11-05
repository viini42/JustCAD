#pragma once

#include "geom/vec2.hpp"

namespace jcad::geom
{

  /** 2D position. Kept apart from Vec2 to preserve affine semantics:
   *  point - point = vector, point + vector = point, point + point does not exist. */
  template <typename T>
  struct Point2
  {
    T x{};
    T y{};

    constexpr Vec2<T> operator-(const Point2& other) const { return { x - other.x, y - other.y }; }
    constexpr Point2 operator+(const Vec2<T>& v) const { return { x + v.x, y + v.y }; }
    constexpr Point2 operator-(const Vec2<T>& v) const { return { x - v.x, y - v.y }; }

    constexpr Point2& operator+=(const Vec2<T>& v)
    {
      x += v.x;
      y += v.y;
      return *this;
    }

    /** Exact, bitwise equality. Use the kernel's Equal() for a tolerant comparison. */
    constexpr bool operator==(const Point2&) const = default;
  };

  /** Affine interpolation: (1 - t) * a + t * b. */
  template <typename T>
  [[nodiscard]] constexpr Point2<T> Lerp(const Point2<T>& a, const Point2<T>& b, const T& t)
  {
    return a + (b - a) * t;
  }

  template <typename T>
  [[nodiscard]] constexpr T SquaredDistance(const Point2<T>& a, const Point2<T>& b)
  {
    return SquaredLength(b - a);
  }

} // namespace jcad::geom
