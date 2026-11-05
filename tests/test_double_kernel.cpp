#include "geom/double_kernel.hpp"
#include "geom/scoped_tolerance.hpp"

#include <gtest/gtest.h>

using jcad::geom::DoubleKernel;
using jcad::geom::ScopedTolerance;
using jcad::geom::Sign;
using Point2d = DoubleKernel::Point2;

TEST(DoubleKernel, OrientationPointLeftOfLineReturnsPositive)
{
  EXPECT_EQ(
    DoubleKernel::Orientation(Point2d{ 0.0, 0.0 }, Point2d{ 1.0, 0.0 }, Point2d{ 0.5, 1.0 }),
    Sign::POSITIVE);
}

TEST(DoubleKernel, OrientationPointRightOfLineReturnsNegative)
{
  EXPECT_EQ(
    DoubleKernel::Orientation(Point2d{ 0.0, 0.0 }, Point2d{ 1.0, 0.0 }, Point2d{ 0.5, -1.0 }),
    Sign::NEGATIVE);
}

TEST(DoubleKernel, OrientationCollinearPointReturnsZero)
{
  EXPECT_EQ(
    DoubleKernel::Orientation(Point2d{ 0.0, 0.0 }, Point2d{ 1.0, 0.0 }, Point2d{ 5.0, 0.0 }),
    Sign::ZERO);
}

TEST(DoubleKernel, OrientationDegenerateSegmentReturnsZero)
{
  EXPECT_EQ(
    DoubleKernel::Orientation(Point2d{ 0.0, 0.0 }, Point2d{ 0.0, 0.0 }, Point2d{ 5.0, 3.0 }),
    Sign::ZERO);
}

TEST(DoubleKernel, OrientationToleranceIsDistanceIndependentOfSegmentLength)
{
  const ScopedTolerance tolerance{ 1e-3 };
  const Point2d origin{ 0.0, 0.0 };
  const Point2d near_line{ 0.5, 0.5e-3 };
  EXPECT_EQ(DoubleKernel::Orientation(origin, Point2d{ 1.0, 0.0 }, near_line), Sign::ZERO);
  EXPECT_EQ(DoubleKernel::Orientation(origin, Point2d{ 1e6, 0.0 }, near_line), Sign::ZERO);
  EXPECT_EQ(DoubleKernel::Orientation(origin, Point2d{ 1e6, 0.0 }, Point2d{ 0.5, 2e-3 }),
            Sign::POSITIVE);
}

TEST(DoubleKernel, EqualPointsWithinToleranceReturnsTrue)
{
  const ScopedTolerance tolerance{ 1e-6 };
  EXPECT_TRUE(DoubleKernel::Equal(Point2d{ 1.0, 1.0 }, Point2d{ 1.0 + 5e-7, 1.0 }));
  EXPECT_FALSE(DoubleKernel::Equal(Point2d{ 1.0, 1.0 }, Point2d{ 1.0 + 2e-6, 1.0 }));
}

TEST(DoubleKernel, CompareXYOrdersByXThenY)
{
  EXPECT_EQ(DoubleKernel::CompareXY(Point2d{ 0.0, 0.0 }, Point2d{ 1.0, -5.0 }), Sign::NEGATIVE);
  EXPECT_EQ(DoubleKernel::CompareXY(Point2d{ 1.0, 2.0 }, Point2d{ 1.0, 1.0 }), Sign::POSITIVE);
}

TEST(DoubleKernel, CompareXYPointsWithinToleranceReturnsZero)
{
  const ScopedTolerance tolerance{ 1e-6 };
  EXPECT_EQ(DoubleKernel::CompareXY(Point2d{ 1.0, 1.0 }, Point2d{ 1.0, 1.0 + 1e-7 }), Sign::ZERO);
}

TEST(DoubleKernel, OnSegmentInteriorAndEndpointsReturnsTrue)
{
  const Point2d a{ 0.0, 0.0 };
  const Point2d b{ 2.0, 2.0 };
  EXPECT_TRUE(DoubleKernel::OnSegment(a, b, Point2d{ 1.0, 1.0 }));
  EXPECT_TRUE(DoubleKernel::OnSegment(a, b, a));
  EXPECT_TRUE(DoubleKernel::OnSegment(a, b, b));
}

TEST(DoubleKernel, OnSegmentCollinearOutsideReturnsFalse)
{
  EXPECT_FALSE(
    DoubleKernel::OnSegment(Point2d{ 0.0, 0.0 }, Point2d{ 2.0, 2.0 }, Point2d{ 3.0, 3.0 }));
}

TEST(DoubleKernel, OnSegmentOffLineReturnsFalse)
{
  EXPECT_FALSE(
    DoubleKernel::OnSegment(Point2d{ 0.0, 0.0 }, Point2d{ 2.0, 2.0 }, Point2d{ 1.0, 1.5 }));
}

TEST(DoubleKernelDeathTest, SetToleranceNegativeValueAsserts)
{
  EXPECT_DEBUG_DEATH(DoubleKernel::SetTolerance(-1.0), "tolerance must be non-negative");
}
