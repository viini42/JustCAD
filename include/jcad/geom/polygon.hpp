#pragma once

#include "geom/kernel.hpp"
#include "geom/point2.hpp"
#include "geom/sign.hpp"
#include "geom/vec2.hpp"

#include <cstddef>
#include <vector>

namespace jcad::geom
{

  enum class PointLocation
  {
    INSIDE,
    OUTSIDE,
    BOUNDARY
  };

  /** The signed area of a closed polygon; positive when counter-clockwise. */
  template <typename T>
  [[nodiscard]] T SignedArea(const std::vector<Point2<T>>& polygon)
  {
    T twice{};
    const std::size_t count{ polygon.size() };
    for (std::size_t i{ 0 }; i < count; ++i)
    {
      const Point2<T>& a{ polygon[i] };
      const Point2<T>& b{ polygon[(i + 1) % count] };
      twice += a.x * b.y - b.x * a.y;
    }
    return twice / T{ 2 };
  }

  /**
   * Where `p` lies relative to a closed polygon (any orientation, possibly with
   * repeated vertices), by its winding number. Every decision goes through the kernel.
   */
  template <kernel K>
  [[nodiscard]] PointLocation LocatePoint(const std::vector<typename K::Point2>& polygon,
                                          const typename K::Point2& p)
  {
    int winding{ 0 };
    const std::size_t count{ polygon.size() };
    for (std::size_t i{ 0 }; i < count; ++i)
    {
      const typename K::Point2& a{ polygon[i] };
      const typename K::Point2& b{ polygon[(i + 1) % count] };
      if (K::OnSegment(a, b, p))
        return PointLocation::BOUNDARY;
      // Half-open rule on y, so a vertex at p's height is counted once.
      const bool a_below{ a.y <= p.y };
      const bool b_below{ b.y <= p.y };
      if (a_below && !b_below && K::Orientation(a, b, p) == Sign::POSITIVE)
        ++winding;
      if (!a_below && b_below && K::Orientation(a, b, p) == Sign::NEGATIVE)
        --winding;
    }
    return winding != 0 ? PointLocation::INSIDE : PointLocation::OUTSIDE;
  }

} // namespace jcad::geom
