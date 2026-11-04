#pragma once

#include "geom/point2.hpp"
#include "geom/sign.hpp"
#include "geom/vec2.hpp"

namespace jcad::geom
{

  /**
   * Floating-point kernel with one global distance tolerance.
   *
   * The tolerance is a distance in the plane: two points are equal when they are
   * at most Tolerance() apart, and three points are collinear when c is at most
   * Tolerance() away from the line a->b, regardless of how long a->b is.
   */
  class DoubleKernel
  {
  public:
    static constexpr double DEFAULT_TOLERANCE{ 1e-9 };

    using Scalar = double;
    using Point2 = geom::Point2<double>;
    using Vec2 = geom::Vec2<double>;

    [[nodiscard]] static double Tolerance() noexcept;

    /** Not thread-safe: it changes the tolerance for every caller. Must be >= 0. */
    static void SetTolerance(double tolerance);

    [[nodiscard]] static Sign SignOf(double value) noexcept;
    [[nodiscard]] static Sign Orientation(Point2 a, Point2 b, Point2 c) noexcept;
    [[nodiscard]] static bool Equal(Point2 a, Point2 b) noexcept;
    [[nodiscard]] static Sign CompareXY(Point2 a, Point2 b) noexcept;
    [[nodiscard]] static bool OnSegment(Point2 a, Point2 b, Point2 c) noexcept;

  private:
    static double s_tolerance;
  };

} // namespace jcad::geom
