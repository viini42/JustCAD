#pragma once

namespace jcad::geom
{

  /** Result of a geometric predicate. Topological decisions are taken from a Sign
   *  returned by the kernel, never by comparing scalars against an epsilon. */
  enum class Sign : int
  {
    NEGATIVE = -1,
    ZERO = 0,
    POSITIVE = 1
  };

  [[nodiscard]] constexpr Sign operator-(Sign sign) noexcept
  {
    return static_cast<Sign>(-static_cast<int>(sign));
  }

} // namespace jcad::geom
