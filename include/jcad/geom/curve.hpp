#pragma once

#include "geom/bbox.hpp"
#include "geom/bezier.hpp"
#include "geom/elliptic_arc.hpp"
#include "geom/line_segment.hpp"
#include "geom/point2.hpp"
#include "geom/vec2.hpp"

#include <utility>
#include <variant>
#include <vector>

namespace jcad::geom
{

  /**
   * The geometry of an edge, parameterized over t in [0, 1]. A variant rather than a
   * class hierarchy: no allocation for the simple kinds, and easy to copy and store.
   */
  using Curve = std::variant<LineSegment, Bezier, EllipticArc>;

  struct CurveSample
  {
    double t{};
    Point2<double> point{};
  };

  [[nodiscard]] Point2<double> Eval(const Curve& curve, double t);
  [[nodiscard]] Vec2<double> Tangent(const Curve& curve, double t);
  [[nodiscard]] Point2<double> StartPoint(const Curve& curve);
  [[nodiscard]] Point2<double> EndPoint(const Curve& curve);

  /** The same curve run backwards: Eval(Reversed(c), t) == Eval(c, 1 - t). */
  [[nodiscard]] Curve Reversed(const Curve& curve);

  /** The pieces over [0, t] and [t, 1], each reparameterized to [0, 1]. */
  [[nodiscard]] std::pair<Curve, Curve> Split(const Curve& curve, double t);

  /** The piece over [from, to] (from < to), reparameterized to [0, 1]. */
  [[nodiscard]] Curve SubCurve(const Curve& curve, double from, double to);

  /** Points along the curve, in increasing t and including both ends, such that the
   *  polyline through them stays within `tolerance` of the curve. */
  [[nodiscard]] std::vector<CurveSample> Sample(const Curve& curve, double tolerance);
  [[nodiscard]] std::vector<Point2<double>> Tessellate(const Curve& curve, double tolerance);

  /** A box containing the curve (not necessarily the tightest one). */
  [[nodiscard]] BBox<double> Bounds(const Curve& curve);

  /** The parameter of the curve point closest to `point`. */
  [[nodiscard]] double ClosestParam(const Curve& curve, Point2<double> point);

  /**
   * The circular arc from `start` through `through` to `end`, or a segment from
   * `start` to `end` when the three points are collinear.
   */
  [[nodiscard]] Curve
  ArcThroughPoints(Point2<double> start, Point2<double> through, Point2<double> end);

} // namespace jcad::geom
