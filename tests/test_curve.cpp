#include "geom/curve.hpp"

#include <cmath>
#include <gtest/gtest.h>
#include <numbers>

using jcad::geom::Bezier;
using jcad::geom::Curve;
using jcad::geom::EllipticArc;
using jcad::geom::LineSegment;
using Point2 = jcad::geom::Point2<double>;

namespace
{

  void ExpectNear(Point2 actual, Point2 expected)
  {
    EXPECT_NEAR(actual.x, expected.x, 1e-9);
    EXPECT_NEAR(actual.y, expected.y, 1e-9);
  }

  const Curve QUADRATIC{ Bezier{ { { 0.0, 0.0 }, { 1.0, 2.0 }, { 2.0, 0.0 } } } };
  const Curve QUARTER{ EllipticArc{ { 0.0, 0.0 }, 2.0, 1.0, 0.0, 0.0, std::numbers::pi / 2.0 } };

} // namespace

TEST(Curve, EvalBezierMidpointUsesDeCasteljau)
{
  ExpectNear(jcad::geom::Eval(QUADRATIC, 0.5), Point2{ 1.0, 1.0 });
  ExpectNear(jcad::geom::EndPoint(QUADRATIC), Point2{ 2.0, 0.0 });
}

TEST(Curve, EvalArcFollowsEllipse)
{
  ExpectNear(jcad::geom::StartPoint(QUARTER), Point2{ 2.0, 0.0 });
  ExpectNear(jcad::geom::EndPoint(QUARTER), Point2{ 0.0, 1.0 });
}

TEST(Curve, TangentOfBezierAtStartPointsToSecondControlPoint)
{
  const auto tangent{ jcad::geom::Tangent(QUADRATIC, 0.0) };
  EXPECT_NEAR(tangent.x, 2.0, 1e-12);
  EXPECT_NEAR(tangent.y, 4.0, 1e-12);
}

TEST(Curve, SplitPiecesMeetAndKeepShape)
{
  for (const Curve& curve :
       { QUADRATIC, QUARTER, Curve{ LineSegment{ { 0.0, 0.0 }, { 4.0, 2.0 } } } })
  {
    const auto [left, right]{ jcad::geom::Split(curve, 0.25) };
    ExpectNear(jcad::geom::EndPoint(left), jcad::geom::Eval(curve, 0.25));
    ExpectNear(jcad::geom::StartPoint(right), jcad::geom::Eval(curve, 0.25));
    ExpectNear(jcad::geom::Eval(left, 0.5), jcad::geom::Eval(curve, 0.125));
    ExpectNear(jcad::geom::Eval(right, 0.5), jcad::geom::Eval(curve, 0.625));
  }
}

TEST(Curve, SubCurveMatchesOriginalRange)
{
  const Curve piece{ jcad::geom::SubCurve(QUADRATIC, 0.2, 0.6) };
  ExpectNear(jcad::geom::StartPoint(piece), jcad::geom::Eval(QUADRATIC, 0.2));
  ExpectNear(jcad::geom::EndPoint(piece), jcad::geom::Eval(QUADRATIC, 0.6));
}

TEST(Curve, ReversedRunsBackwards)
{
  for (const Curve& curve : { QUADRATIC, QUARTER })
    ExpectNear(jcad::geom::Eval(jcad::geom::Reversed(curve), 0.3), jcad::geom::Eval(curve, 0.7));
}

TEST(Curve, TessellateStaysWithinTolerance)
{
  const auto points{ jcad::geom::Tessellate(QUARTER, 1e-3) };
  ASSERT_GE(points.size(), 9U);
  for (std::size_t i{ 1 }; i < points.size(); ++i)
  {
    const Point2 mid{ jcad::geom::Lerp(points[i - 1], points[i], 0.5) };
    const double t{ jcad::geom::ClosestParam(QUARTER, mid) };
    EXPECT_LE(Length(jcad::geom::Eval(QUARTER, t) - mid), 1e-3);
  }
}

TEST(Curve, ClosestParamOnCurvePointRecoversParameter)
{
  EXPECT_NEAR(jcad::geom::ClosestParam(QUARTER, jcad::geom::Eval(QUARTER, 0.37)), 0.37, 1e-9);
  EXPECT_NEAR(jcad::geom::ClosestParam(QUADRATIC, Point2{ 1.0, 5.0 }), 0.5, 1e-9);
}

TEST(Curve, BoundsContainCurve)
{
  const auto box{ jcad::geom::Bounds(QUARTER) };
  for (const Point2& point : jcad::geom::Tessellate(QUARTER, 1e-3))
    EXPECT_TRUE(box.Inflated(1e-12).Contains(point));
}

TEST(Curve, ArcThroughPointsPassesThroughAllThree)
{
  const Point2 start{ 1.0, 0.0 };
  const Point2 through{ 0.0, 1.0 };
  const Point2 end{ -1.0, 0.0 };
  const Curve arc{ jcad::geom::ArcThroughPoints(start, through, end) };
  ASSERT_TRUE(std::holds_alternative<EllipticArc>(arc));
  ExpectNear(jcad::geom::StartPoint(arc), start);
  ExpectNear(jcad::geom::EndPoint(arc), end);
  ExpectNear(jcad::geom::Eval(arc, 0.5), through);
  const Curve clockwise{ jcad::geom::ArcThroughPoints(start, Point2{ 0.0, -1.0 }, end) };
  ExpectNear(jcad::geom::Eval(clockwise, 0.5), Point2{ 0.0, -1.0 });
}

TEST(Curve, ArcThroughCollinearPointsIsSegment)
{
  const Curve line{
    jcad::geom::ArcThroughPoints(Point2{ 0.0, 0.0 }, Point2{ 1.0, 1.0 }, Point2{ 2.0, 2.0 })
  };
  EXPECT_TRUE(std::holds_alternative<LineSegment>(line));
}
