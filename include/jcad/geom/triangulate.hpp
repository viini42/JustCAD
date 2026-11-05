#pragma once

#include "geom/kernel.hpp"
#include "geom/sign.hpp"

#include <array>
#include <cstddef>
#include <vector>

namespace jcad::geom
{

  using Triangle = std::array<std::size_t, 3>;

  namespace detail
  {

    /** Whether no other polygon point lies inside or on the triangle (a, b, c). */
    template <kernel K>
    bool IsEmptyTriangle(const std::vector<typename K::Point2>& polygon,
                         const std::vector<std::size_t>& remaining,
                         const Triangle& triangle)
    {
      const typename K::Point2& a{ polygon[triangle[0]] };
      const typename K::Point2& b{ polygon[triangle[1]] };
      const typename K::Point2& c{ polygon[triangle[2]] };
      for (const std::size_t index : remaining)
      {
        const typename K::Point2& p{ polygon[index] };
        if (K::Equal(p, a) || K::Equal(p, b) || K::Equal(p, c))
          continue;
        if (K::Orientation(a, b, p) != Sign::NEGATIVE &&
            K::Orientation(b, c, p) != Sign::NEGATIVE && K::Orientation(c, a, p) != Sign::NEGATIVE)
          return false;
      }
      return true;
    }

  } // namespace detail

  /**
   * Ear-clipping triangulation of a counter-clockwise simple polygon, as indices
   * into `polygon`. Degenerate parts (repeated points, zero-width spikes such as a
   * dangling edge inside a face) produce no triangles. O(n^3), meant for display.
   */
  template <kernel K>
  [[nodiscard]] std::vector<Triangle> Triangulate(const std::vector<typename K::Point2>& polygon)
  {
    std::vector<std::size_t> remaining;
    for (std::size_t i{ 0 }; i < polygon.size(); ++i)
      remaining.push_back(i);
    std::vector<Triangle> triangles;
    std::size_t i{ 0 };
    std::size_t misses{ 0 };
    while (remaining.size() >= 3 && misses < remaining.size())
    {
      const std::size_t count{ remaining.size() };
      const Triangle corner{ remaining[(i + count - 1) % count],
                             remaining[i % count],
                             remaining[(i + 1) % count] };
      const Sign turn{ K::Orientation(polygon[corner[0]], polygon[corner[1]], polygon[corner[2]]) };
      const bool ear{ turn == Sign::POSITIVE &&
                      detail::IsEmptyTriangle<K>(polygon, remaining, corner) };
      if (turn == Sign::ZERO || ear)
      {
        if (ear)
          triangles.push_back(corner);
        remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i % count));
        misses = 0;
        continue;
      }
      i = (i + 1) % count;
      ++misses;
    }
    return triangles;
  }

} // namespace jcad::geom
