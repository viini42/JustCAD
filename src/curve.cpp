#include "geom/curve.hpp"

#include "core/assert.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>

namespace
{

  using jcad::geom::BBox;
  using jcad::geom::Bezier;
  using jcad::geom::Curve;
  using jcad::geom::CurveSample;
  using jcad::geom::EllipticArc;
  using jcad::geom::LineSegment;
  using Point2d = jcad::geom::Point2<double>;
  using Vec2d = jcad::geom::Vec2<double>;

  constexpr int MIN_SAMPLE_DEPTH{ 3 };
  constexpr int MAX_SAMPLE_DEPTH{ 14 };
  constexpr int CLOSEST_ITERATIONS{ 12 };

  template <typename... Ts>
  struct Overloaded : Ts...
  {
    using Ts::operator()...;
  };

  Vec2d Rotate(Vec2d v, double angle)
  {
    const double c{ std::cos(angle) };
    const double s{ std::sin(angle) };
    return Vec2d{ c * v.x - s * v.y, s * v.x + c * v.y };
  }

  Point2d DeCasteljau(std::vector<Point2d> points, double t)
  {
    for (std::size_t level{ 1 }; level < points.size(); ++level)
    {
      for (std::size_t i{ 0 }; i + level < points.size(); ++i)
        points[i] = Lerp(points[i], points[i + 1], t);
    }
    return points.front();
  }

  Point2d EvalArc(const EllipticArc& arc, double t)
  {
    const double angle{ arc.start_angle + arc.sweep * t };
    const Vec2d local{ arc.radius_x * std::cos(angle), arc.radius_y * std::sin(angle) };
    return arc.center + Rotate(local, arc.rotation);
  }

  Vec2d ArcTangent(const EllipticArc& arc, double t)
  {
    const double angle{ arc.start_angle + arc.sweep * t };
    const Vec2d local{ -arc.radius_x * std::sin(angle), arc.radius_y * std::cos(angle) };
    return Rotate(local, arc.rotation) * arc.sweep;
  }

  Vec2d BezierTangent(const Bezier& bezier, double t)
  {
    const std::vector<Point2d>& points{ bezier.control_points };
    if (points.size() < 2)
      return Vec2d{};
    // The derivative is a Bézier of one degree less on n * (P[i+1] - P[i]); its
    // control "points" are vectors, stored as offsets from the origin.
    std::vector<Point2d> derivative;
    const auto degree{ static_cast<double>(points.size() - 1) };
    for (std::size_t i{ 0 }; i + 1 < points.size(); ++i)
      derivative.push_back(Point2d{} + (points[i + 1] - points[i]) * degree);
    return DeCasteljau(derivative, t) - Point2d{};
  }

  std::pair<Bezier, Bezier> SplitBezier(const Bezier& bezier, double t)
  {
    std::vector<Point2d> points{ bezier.control_points };
    std::vector<Point2d> left{ points.front() };
    std::vector<Point2d> right{ points.back() };
    for (std::size_t level{ 1 }; level < points.size(); ++level)
    {
      for (std::size_t i{ 0 }; i + level < points.size(); ++i)
        points[i] = Lerp(points[i], points[i + 1], t);
      left.push_back(points.front());
      right.push_back(points[points.size() - 1 - level]);
    }
    std::ranges::reverse(right);
    return { Bezier{ std::move(left) }, Bezier{ std::move(right) } };
  }

  /** Adaptive sampling: halves a parameter range until its midpoint is close
   *  enough to the chord (with a few forced levels, so curls are not missed). */
  class Sampler
  {
  public:
    Sampler(const Curve& curve, double tolerance) : m_curve{ curve }, m_tolerance{ tolerance } {}

    void Range(CurveSample from, CurveSample to, int depth)
    {
      const double mid_t{ (from.t + to.t) / 2.0 };
      const Point2d mid{ jcad::geom::Eval(m_curve, mid_t) };
      const double deviation{ Length(mid - Lerp(from.point, to.point, 0.5)) };
      if (depth >= MAX_SAMPLE_DEPTH || (depth >= MIN_SAMPLE_DEPTH && deviation <= m_tolerance))
      {
        m_samples.push_back(to);
        return;
      }
      const CurveSample middle{ mid_t, mid };
      Range(from, middle, depth + 1);
      Range(middle, to, depth + 1);
    }

    std::vector<CurveSample> Take() { return std::move(m_samples); }

  private:
    const Curve& m_curve;
    double m_tolerance;
    std::vector<CurveSample> m_samples;
  };

  /** Parameter of the point of segment [a, b] closest to p, mapped to [ta, tb]. */
  double ProjectOnSegment(const CurveSample& a, const CurveSample& b, Point2d p)
  {
    const Vec2d ab{ b.point - a.point };
    const double length_squared{ SquaredLength(ab) };
    const double s{ length_squared > 0.0
                      ? std::clamp(Dot(p - a.point, ab) / length_squared, 0.0, 1.0)
                      : 0.0 };
    return a.t + (b.t - a.t) * s;
  }

} // namespace

jcad::geom::Point2<double> jcad::geom::Eval(const Curve& curve, double t)
{
  return std::visit(
    Overloaded{
      [&](const LineSegment& line) { return Lerp(line.start, line.end, t); },
      [&](const Bezier& bezier) { return DeCasteljau(bezier.control_points, t); },
      [&](const EllipticArc& arc) { return EvalArc(arc, t); },
    },
    curve);
}

jcad::geom::Vec2<double> jcad::geom::Tangent(const Curve& curve, double t)
{
  return std::visit(
    Overloaded{
      [](const LineSegment& line) { return line.end - line.start; },
      [&](const Bezier& bezier) { return BezierTangent(bezier, t); },
      [&](const EllipticArc& arc) { return ArcTangent(arc, t); },
    },
    curve);
}

jcad::geom::Point2<double> jcad::geom::StartPoint(const Curve& curve)
{
  return Eval(curve, 0.0);
}

jcad::geom::Point2<double> jcad::geom::EndPoint(const Curve& curve)
{
  return Eval(curve, 1.0);
}

jcad::geom::Curve jcad::geom::Reversed(const Curve& curve)
{
  return std::visit(
    Overloaded{
      [](const LineSegment& line) -> Curve { return LineSegment{ line.end, line.start }; },
      [](Bezier bezier) -> Curve
      {
        std::ranges::reverse(bezier.control_points);
        return bezier;
      },
      [](EllipticArc arc) -> Curve
      {
        arc.start_angle += arc.sweep;
        arc.sweep = -arc.sweep;
        return arc;
      },
    },
    curve);
}

std::pair<jcad::geom::Curve, jcad::geom::Curve> jcad::geom::Split(const Curve& curve, double t)
{
  JCAD_ASSERT(t >= 0.0 && t <= 1.0, "split parameter {} is outside [0, 1]", t);
  return std::visit(
    Overloaded{
      [&](const LineSegment& line) -> std::pair<Curve, Curve>
      {
        const Point2d middle{ Lerp(line.start, line.end, t) };
        return { LineSegment{ line.start, middle }, LineSegment{ middle, line.end } };
      },
      [&](const Bezier& bezier) -> std::pair<Curve, Curve>
      {
        auto [left, right]{ SplitBezier(bezier, t) };
        return { std::move(left), std::move(right) };
      },
      [&](const EllipticArc& arc) -> std::pair<Curve, Curve>
      {
        EllipticArc first{ arc };
        first.sweep = arc.sweep * t;
        EllipticArc second{ arc };
        second.start_angle = arc.start_angle + first.sweep;
        second.sweep = arc.sweep - first.sweep;
        return { first, second };
      },
    },
    curve);
}

jcad::geom::Curve jcad::geom::SubCurve(const Curve& curve, double from, double to)
{
  JCAD_ASSERT(from < to, "empty sub-curve [{}, {}]", from, to);
  const Curve tail{ from > 0.0 ? Split(curve, from).second : curve };
  const double end{ (to - from) / (1.0 - from) };
  return end < 1.0 ? Split(tail, end).first : tail;
}

std::vector<jcad::geom::CurveSample> jcad::geom::Sample(const Curve& curve, double tolerance)
{
  const CurveSample first{ 0.0, StartPoint(curve) };
  const CurveSample last{ 1.0, EndPoint(curve) };
  if (std::holds_alternative<LineSegment>(curve))
    return { first, last };
  Sampler sampler{ curve, tolerance };
  sampler.Range(first, last, 0);
  std::vector<CurveSample> samples{ sampler.Take() };
  samples.insert(samples.begin(), first);
  return samples;
}

std::vector<jcad::geom::Point2<double>> jcad::geom::Tessellate(const Curve& curve, double tolerance)
{
  std::vector<Point2d> points;
  for (const CurveSample& sample : Sample(curve, tolerance))
    points.push_back(sample.point);
  return points;
}

jcad::geom::BBox<double> jcad::geom::Bounds(const Curve& curve)
{
  return std::visit(
    Overloaded{
      [](const LineSegment& line) { return BBox<double>::Of(line.start, line.end); },
      [](const Bezier& bezier)
      {
        // The curve lies in the convex hull of its control points.
        BBox<double> box{};
        for (const Point2d& point : bezier.control_points)
          box.Expand(point);
        return box;
      },
      [](const EllipticArc& arc)
      {
        // Box of the whole ellipse: conservative but cheap.
        const double c{ std::cos(arc.rotation) };
        const double s{ std::sin(arc.rotation) };
        const Vec2d half{ std::hypot(arc.radius_x * c, arc.radius_y * s),
                          std::hypot(arc.radius_x * s, arc.radius_y * c) };
        return BBox<double>::Of(arc.center - half, arc.center + half);
      },
    },
    curve);
}

double jcad::geom::ClosestParam(const Curve& curve, Point2d point)
{
  const BBox<double> box{ Bounds(curve) };
  const double size{ std::max(box.Width(), box.Height()) };
  const std::vector<CurveSample> samples{ Sample(curve, std::max(size, 1e-12) * 1e-3) };
  double best_t{ 0.0 };
  double best_distance{ std::numeric_limits<double>::infinity() };
  for (std::size_t i{ 1 }; i < samples.size(); ++i)
  {
    const double t{ ProjectOnSegment(samples[i - 1], samples[i], point) };
    const double distance{ SquaredDistance(Eval(curve, t), point) };
    if (distance < best_distance)
    {
      best_t = t;
      best_distance = distance;
    }
  }
  // Gauss-Newton on (C(t) - p) . C'(t) = 0.
  for (int i{ 0 }; i < CLOSEST_ITERATIONS; ++i)
  {
    const Vec2d tangent{ Tangent(curve, best_t) };
    const double speed{ SquaredLength(tangent) };
    if (speed <= 0.0)
      break;
    best_t = std::clamp(best_t - Dot(Eval(curve, best_t) - point, tangent) / speed, 0.0, 1.0);
  }
  return best_t;
}

jcad::geom::Curve jcad::geom::ArcThroughPoints(Point2d start, Point2d through, Point2d end)
{
  const Vec2d ab{ through - start };
  const Vec2d ac{ end - start };
  const double twice_area{ Cross(ab, ac) };
  if (std::abs(twice_area) <= 1e-12 * SquaredLength(ac))
    return LineSegment{ start, end };
  // Circumcenter of the triangle (start, through, end), relative to start.
  const double d{ 2.0 * twice_area };
  const Vec2d offset{ (ac.y * SquaredLength(ab) - ab.y * SquaredLength(ac)) / d,
                      (ab.x * SquaredLength(ac) - ac.x * SquaredLength(ab)) / d };
  const Point2d center{ start + offset };
  const double radius{ Length(offset) };
  const auto angle{ [&](Point2d p) { return std::atan2(p.y - center.y, p.x - center.x); } };
  const auto ccw{ [](double from, double to)
                  {
                    const double turn{ std::fmod(to - from, 2.0 * std::numbers::pi) };
                    return turn < 0.0 ? turn + 2.0 * std::numbers::pi : turn;
                  } };
  const double start_angle{ angle(start) };
  const double to_end{ ccw(start_angle, angle(end)) };
  const double to_through{ ccw(start_angle, angle(through)) };
  const double sweep{ to_through < to_end ? to_end : to_end - 2.0 * std::numbers::pi };
  return EllipticArc{ center, radius, radius, 0.0, start_angle, sweep };
}
