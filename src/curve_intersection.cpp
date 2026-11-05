#include "geom/curve_intersection.hpp"

#include "geom/double_kernel.hpp"
#include "geom/segment_crossing.hpp"
#include "geom/sign.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <optional>
#include <utility>

namespace
{

  using jcad::geom::Curve;
  using jcad::geom::CurveIntersection;
  using jcad::geom::CurveSample;
  using jcad::geom::DoubleKernel;
  using jcad::geom::Sign;
  using Point2d = jcad::geom::Point2<double>;
  using Vec2d = jcad::geom::Vec2<double>;

  constexpr int NEWTON_ITERATIONS{ 30 };

  /** Closed-segment test: the segments share at least one point. */
  bool SegmentsMeet(Point2d a, Point2d b, Point2d c, Point2d d)
  {
    const Sign c_side{ DoubleKernel::Orientation(a, b, c) };
    const Sign d_side{ DoubleKernel::Orientation(a, b, d) };
    const Sign a_side{ DoubleKernel::Orientation(c, d, a) };
    const Sign b_side{ DoubleKernel::Orientation(c, d, b) };
    if (c_side == Sign::ZERO && d_side == Sign::ZERO)
      return false; // collinear overlap: not a crossing
    return c_side != d_side && a_side != b_side;
  }

  /** Where the lines through [a, b] and [c, d] meet, as parameters on each. */
  std::pair<double, double> LineParameters(Point2d a, Point2d b, Point2d c, Point2d d)
  {
    const Vec2d ab{ b - a };
    const Vec2d cd{ d - c };
    const double denominator{ Cross(ab, cd) };
    if (denominator == 0.0)
      return { 0.0, 0.0 };
    return { Cross(c - a, cd) / denominator, Cross(c - a, ab) / denominator };
  }

  struct Guess
  {
    double s{};
    double t{};
  };

  std::optional<CurveIntersection>
  Refine(const Curve& a, const Curve& b, Guess guess, double tolerance)
  {
    double s{ guess.s };
    double t{ guess.t };
    for (int i{ 0 }; i < NEWTON_ITERATIONS; ++i)
    {
      const Vec2d residual{ jcad::geom::Eval(a, s) - jcad::geom::Eval(b, t) };
      const Vec2d da{ jcad::geom::Tangent(a, s) };
      const Vec2d db{ jcad::geom::Tangent(b, t) };
      // Solve [da, -db] * (ds, dt) = -residual.
      const double determinant{ -Cross(da, db) };
      if (determinant == 0.0)
        break;
      const double ds{ (-residual.x * -db.y + db.x * -residual.y) / determinant };
      const double dt{ (da.x * -residual.y - da.y * -residual.x) / determinant };
      s = std::clamp(s + ds, 0.0, 1.0);
      t = std::clamp(t + dt, 0.0, 1.0);
    }
    const Point2d point{ jcad::geom::Eval(a, s) };
    if (Length(point - jcad::geom::Eval(b, t)) > tolerance)
      return std::nullopt;
    return CurveIntersection{ s, t, point };
  }

  std::vector<CurveIntersection> IntersectSegments(const Curve& a, const Curve& b)
  {
    const auto& p{ std::get<jcad::geom::LineSegment>(a) };
    const auto& q{ std::get<jcad::geom::LineSegment>(b) };
    const std::optional<Point2d> point{
      jcad::geom::ProperCrossing<DoubleKernel>(p.start, p.end, q.start, q.end)
    };
    if (!point)
      return {};
    const auto [s, t]{ LineParameters(p.start, p.end, q.start, q.end) };
    return { CurveIntersection{ s, t, *point } };
  }

  double PointSegmentDistance(Point2d p, Point2d a, Point2d b)
  {
    const Vec2d ab{ b - a };
    const double squared{ Dot(ab, ab) };
    if (squared == 0.0)
      return Length(p - a);
    const double u{ std::clamp(Dot(p - a, ab) / squared, 0.0, 1.0) };
    return Length(p - (a + ab * u));
  }

  /** Distance between closed segments [a, b] and [c, d] that do not cross. */
  double SegmentDistance(Point2d a, Point2d b, Point2d c, Point2d d)
  {
    return std::min({ PointSegmentDistance(a, c, d),
                      PointSegmentDistance(b, c, d),
                      PointSegmentDistance(c, a, b),
                      PointSegmentDistance(d, a, b) });
  }

  constexpr int SEARCH_ITERATIONS{ 100 };

  /** Ternary search of a unimodal `cost` over [from, to]; returns the minimizer. */
  template <typename Cost>
  double Minimize(double from, double to, const Cost& cost)
  {
    for (int i{ 0 }; i < SEARCH_ITERATIONS; ++i)
    {
      const double left{ from + (to - from) / 3.0 };
      const double right{ to - (to - from) / 3.0 };
      if (cost(left) <= cost(right))
        to = right;
      else
        from = left;
    }
    return (from + to) / 2.0;
  }

  /**
   * Looks for a tangential contact between `a` on [low.s, high.s] and `b` on
   * [low.t, high.t]: the closest pair of points, accepted when they coincide
   * within the kernel tolerance.
   */
  std::optional<CurveIntersection> Touch(const Curve& a, const Curve& b, Guess low, Guess high)
  {
    const auto closest_on_b{ [&](double s)
                             {
                               const Point2d point{ jcad::geom::Eval(a, s) };
                               return Minimize(low.t,
                                               high.t,
                                               [&](double t)
                                               { return Length(jcad::geom::Eval(b, t) - point); });
                             } };
    const auto gap{ [&](double s) {
      return Length(jcad::geom::Eval(a, s) - jcad::geom::Eval(b, closest_on_b(s)));
    } };
    const double s{ Minimize(low.s, high.s, gap) };
    const double t{ closest_on_b(s) };
    const Point2d point{ jcad::geom::Eval(a, s) };
    if (Length(point - jcad::geom::Eval(b, t)) > DoubleKernel::Tolerance())
      return std::nullopt;
    return CurveIntersection{ s, t, point };
  }

  /** Collects the meeting points of two curves, one polyline segment pair at a time. */
  class PairScanner
  {
  public:
    PairScanner(const Curve& a, const Curve& b, double tolerance) :
        m_a{ a }, m_b{ b }, m_tolerance{ tolerance }
    {
    }

    void
    Scan(const CurveSample& a0, const CurveSample& a1, const CurveSample& b0, const CurveSample& b1)
    {
      if (SegmentsMeet(a0.point, a1.point, b0.point, b1.point))
      {
        const auto [u, v]{ LineParameters(a0.point, a1.point, b0.point, b1.point) };
        const Guess guess{ a0.t + (a1.t - a0.t) * u, b0.t + (b1.t - b0.t) * v };
        Add(Refine(m_a, m_b, guess, m_tolerance));
        return;
      }
      if (SegmentDistance(a0.point, a1.point, b0.point, b1.point) <= m_tolerance)
        Add(Touch(m_a, m_b, Guess{ a0.t, b0.t }, Guess{ a1.t, b1.t }));
    }

    [[nodiscard]] std::vector<CurveIntersection> Take() { return std::move(m_found); }

  private:
    void Add(const std::optional<CurveIntersection>& hit)
    {
      if (!hit)
        return;
      const bool known{ std::ranges::any_of(m_found,
                                            [&](const CurveIntersection& other) {
                                              return Length(other.point - hit->point) <=
                                                     m_tolerance;
                                            }) };
      if (!known)
        m_found.push_back(*hit);
    }

    const Curve& m_a;
    const Curve& m_b;
    double m_tolerance;
    std::vector<CurveIntersection> m_found;
  };

} // namespace

std::vector<jcad::geom::CurveIntersection>
jcad::geom::Intersect(const Curve& a, const Curve& b, double tolerance)
{
  if (std::holds_alternative<LineSegment>(a) && std::holds_alternative<LineSegment>(b))
    return IntersectSegments(a, b);
  if (!Bounds(a).Inflated(tolerance).Intersects(Bounds(b).Inflated(tolerance)))
    return {};
  const std::vector<CurveSample> samples_a{ Sample(a, tolerance) };
  const std::vector<CurveSample> samples_b{ Sample(b, tolerance) };
  PairScanner scanner{ a, b, tolerance };
  for (std::size_t i{ 1 }; i < samples_a.size(); ++i)
  {
    for (std::size_t j{ 1 }; j < samples_b.size(); ++j)
      scanner.Scan(samples_a[i - 1], samples_a[i], samples_b[j - 1], samples_b[j]);
  }
  std::vector<CurveIntersection> found{ scanner.Take() };
  std::ranges::sort(found, {}, &CurveIntersection::t_a);
  return found;
}
