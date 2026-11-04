#pragma once

#include "geom/point2.hpp"

#include <algorithm>
#include <limits>

namespace jcad::geom
{

  /** Axis-aligned bounding box. Starts empty (min > max) and grows with Expand(). */
  template <typename T>
  struct BBox
  {
    Point2<T> min{ std::numeric_limits<T>::max(), std::numeric_limits<T>::max() };
    Point2<T> max{ std::numeric_limits<T>::lowest(), std::numeric_limits<T>::lowest() };

    [[nodiscard]] static constexpr BBox Of(const Point2<T>& a, const Point2<T>& b)
    {
      BBox box{};
      box.Expand(a);
      box.Expand(b);
      return box;
    }

    [[nodiscard]] constexpr bool Empty() const { return min.x > max.x || min.y > max.y; }

    constexpr void Expand(const Point2<T>& p)
    {
      min.x = std::min(min.x, p.x);
      min.y = std::min(min.y, p.y);
      max.x = std::max(max.x, p.x);
      max.y = std::max(max.y, p.y);
    }

    constexpr void Expand(const BBox& other)
    {
      if (other.Empty())
        return;
      Expand(other.min);
      Expand(other.max);
    }

    /** The box grown by `margin` on every side. An empty box stays empty. */
    [[nodiscard]] constexpr BBox Inflated(const T& margin) const
    {
      if (Empty())
        return *this;
      return { { min.x - margin, min.y - margin }, { max.x + margin, max.y + margin } };
    }

    [[nodiscard]] constexpr bool Contains(const Point2<T>& p) const
    {
      return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y;
    }

    [[nodiscard]] constexpr bool Intersects(const BBox& other) const
    {
      if (Empty() || other.Empty())
        return false;
      return min.x <= other.max.x && other.min.x <= max.x && min.y <= other.max.y &&
             other.min.y <= max.y;
    }

    [[nodiscard]] constexpr T Width() const { return Empty() ? T{} : max.x - min.x; }
    [[nodiscard]] constexpr T Height() const { return Empty() ? T{} : max.y - min.y; }
  };

} // namespace jcad::geom
