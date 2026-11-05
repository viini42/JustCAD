#pragma once

#include "geom/kernel.hpp"
#include "geom/sign.hpp"
#include "geom/vec2.hpp"

#include <optional>

namespace jcad::geom
{

  /**
   * The point where segments [a, b] and [c, d] cross, if they cross properly: each
   * one has the ends of the other strictly on opposite sides. Touching at an end or
   * overlapping collinearly is not a proper crossing; detect those with OnSegment.
   */
  template <kernel K>
  [[nodiscard]] std::optional<typename K::Point2> ProperCrossing(const typename K::Point2& a,
                                                                 const typename K::Point2& b,
                                                                 const typename K::Point2& c,
                                                                 const typename K::Point2& d)
  {
    const Sign c_side{ K::Orientation(a, b, c) };
    const Sign d_side{ K::Orientation(a, b, d) };
    const Sign a_side{ K::Orientation(c, d, a) };
    const Sign b_side{ K::Orientation(c, d, b) };
    const bool straddle_ab{ c_side != Sign::ZERO && d_side == -c_side };
    const bool straddle_cd{ a_side != Sign::ZERO && b_side == -a_side };
    if (!straddle_ab || !straddle_cd)
      return std::nullopt;
    const typename K::Vec2 ab{ b - a };
    const typename K::Vec2 cd{ d - c };
    const typename K::Scalar t{ Cross(c - a, cd) / Cross(ab, cd) };
    return a + ab * t;
  }

} // namespace jcad::geom
