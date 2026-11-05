#include "geom/curve_intersection.hpp"

#include <gtest/gtest.h>
#include <numbers>

using jcad::geom::Bezier;
using jcad::geom::Curve;
using jcad::geom::EllipticArc;
using jcad::geom::Intersect;
using jcad::geom::LineSegment;
using Point2 = jcad::geom::Point2<double>;

TEST(CurveIntersection, SegmentsCrossingIsExact)
{
  const auto hits{ Intersect(LineSegment{ { 0.0, 0.0 }, { 2.0, 2.0 } },
                             LineSegment{ { 0.0, 2.0 }, { 2.0, 0.0 } },
                             1e-6) };
  ASSERT_EQ(hits.size(), 1U);
  EXPECT_DOUBLE_EQ(hits[0].t_a, 0.5);
  EXPECT_DOUBLE_EQ(hits[0].point.x, 1.0);
}

TEST(CurveIntersection, LineThroughCircleHitsTwice)
{
  const Curve circle{ EllipticArc{ { 0.0, 0.0 }, 1.0, 1.0, 0.0, 0.1, 2.0 * std::numbers::pi } };
  const auto hits{ Intersect(LineSegment{ { -2.0, 0.5 }, { 2.0, 0.5 } }, circle, 1e-6) };
  ASSERT_EQ(hits.size(), 2U);
  EXPECT_NEAR(hits[0].point.x, -std::sqrt(0.75), 1e-12);
  EXPECT_NEAR(hits[1].point.x, std::sqrt(0.75), 1e-12);
  EXPECT_NEAR(hits[0].point.y, 0.5, 1e-12);
}

TEST(CurveIntersection, BezierCrossesBezier)
{
  const Curve up{ Bezier{ { { 0.0, 0.0 }, { 1.0, 2.0 }, { 2.0, 0.0 } } } };
  const Curve down{ Bezier{ { { 0.0, 1.5 }, { 1.0, -0.5 }, { 2.0, 1.5 } } } };
  const auto hits{ Intersect(up, down, 1e-6) };
  ASSERT_EQ(hits.size(), 2U);
  for (const auto& hit : hits)
    EXPECT_LT(Length(jcad::geom::Eval(up, hit.t_a) - jcad::geom::Eval(down, hit.t_b)), 1e-12);
}

TEST(CurveIntersection, DisjointCurvesDoNotMeet)
{
  const Curve arc{ EllipticArc{ { 0.0, 0.0 }, 1.0, 1.0, 0.0, 0.0, std::numbers::pi } };
  EXPECT_TRUE(Intersect(arc, LineSegment{ { -2.0, -1.0 }, { 2.0, -1.0 } }, 1e-6).empty());
}

TEST(CurveIntersection, TangentLineTouchesCircleOnce)
{
  const Curve circle{ EllipticArc{ { 0.0, 0.0 }, 1.0, 1.0, 0.0, 0.0, std::numbers::pi } };
  const auto hits{ Intersect(LineSegment{ { -2.0, 1.0 }, { 2.0, 1.0 } }, circle, 1e-3) };
  ASSERT_EQ(hits.size(), 1U);
  EXPECT_NEAR(hits[0].point.x, 0.0, 1e-6);
  EXPECT_NEAR(hits[0].point.y, 1.0, 1e-9);
}

TEST(CurveIntersection, NearMissIsNotATouch)
{
  const Curve circle{ EllipticArc{ { 0.0, 0.0 }, 1.0, 1.0, 0.0, 0.0, std::numbers::pi } };
  EXPECT_TRUE(Intersect(LineSegment{ { -2.0, 1.0001 }, { 2.0, 1.0001 } }, circle, 1e-3).empty());
}
