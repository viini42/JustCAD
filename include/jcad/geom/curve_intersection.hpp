#pragma once

#include "geom/curve.hpp"
#include "geom/point2.hpp"

#include <vector>

namespace jcad::geom
{

  struct CurveIntersection
  {
    double t_a{};
    double t_b{};
    Point2<double> point{};
  };

  /**
   * Points where curves `a` and `b` meet, sorted by t_a. Two segments are solved
   * exactly; any other pair is found on polylines within `tolerance` of the curves
   * and refined with Newton's method on a(s) = b(t). Overlapping stretches and
   * tangential contacts that the polylines do not show are not reported.
   */
  [[nodiscard]] std::vector<CurveIntersection>
  Intersect(const Curve& a, const Curve& b, double tolerance);

} // namespace jcad::geom
