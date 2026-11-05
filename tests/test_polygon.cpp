#include "geom/double_kernel.hpp"
#include "geom/polygon.hpp"

#include <gtest/gtest.h>
#include <vector>

using jcad::geom::DoubleKernel;
using jcad::geom::LocatePoint;
using jcad::geom::PointLocation;
using jcad::geom::SignedArea;
using Point2d = DoubleKernel::Point2;

namespace
{

  const std::vector<Point2d> SQUARE{ { 0.0, 0.0 }, { 2.0, 0.0 }, { 2.0, 2.0 }, { 0.0, 2.0 } };

} // namespace

TEST(Polygon, SignedAreaCounterClockwiseIsPositive)
{
  EXPECT_DOUBLE_EQ(SignedArea(SQUARE), 4.0);
  const std::vector<Point2d> clockwise(SQUARE.rbegin(), SQUARE.rend());
  EXPECT_DOUBLE_EQ(SignedArea(clockwise), -4.0);
}

TEST(Polygon, LocatePointClassifiesInsideOutsideAndBoundary)
{
  EXPECT_EQ(LocatePoint<DoubleKernel>(SQUARE, Point2d{ 1.0, 1.0 }), PointLocation::INSIDE);
  EXPECT_EQ(LocatePoint<DoubleKernel>(SQUARE, Point2d{ 3.0, 1.0 }), PointLocation::OUTSIDE);
  EXPECT_EQ(LocatePoint<DoubleKernel>(SQUARE, Point2d{ 2.0, 1.0 }), PointLocation::BOUNDARY);
}

TEST(Polygon, LocatePointAtVertexHeightCountsOnce)
{
  const std::vector<Point2d> diamond{ { 1.0, 0.0 }, { 2.0, 1.0 }, { 1.0, 2.0 }, { 0.0, 1.0 } };
  EXPECT_EQ(LocatePoint<DoubleKernel>(diamond, Point2d{ 1.0, 1.0 }), PointLocation::INSIDE);
  EXPECT_EQ(LocatePoint<DoubleKernel>(diamond, Point2d{ -1.0, 1.0 }), PointLocation::OUTSIDE);
}

TEST(Polygon, LocatePointIgnoresNearlyHorizontalEdgeWestOfPoint)
{
  // The bottom edge rises by less than the tolerance, so the kernel calls the
  // point collinear with it; the point is still clearly outside, to the west.
  const std::vector<Point2d> polygon{ { 2.0, 3.0 + 1e-10 }, { 4.0, 3.0 }, { 3.0, 5.0 } };
  EXPECT_EQ(LocatePoint<DoubleKernel>(polygon, Point2d{ 0.0, 3.0 }), PointLocation::OUTSIDE);
  EXPECT_EQ(LocatePoint<DoubleKernel>(polygon, Point2d{ 3.0, 4.0 }), PointLocation::INSIDE);
}
