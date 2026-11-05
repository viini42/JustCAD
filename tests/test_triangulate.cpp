#include "geom/double_kernel.hpp"
#include "geom/polygon.hpp"
#include "geom/triangulate.hpp"

#include <gtest/gtest.h>
#include <vector>

using jcad::geom::DoubleKernel;
using jcad::geom::Triangle;
using jcad::geom::Triangulate;
using Point2d = DoubleKernel::Point2;

namespace
{

  double TotalArea(const std::vector<Point2d>& polygon, const std::vector<Triangle>& triangles)
  {
    double total{ 0.0 };
    for (const Triangle& triangle : triangles)
    {
      const std::vector<Point2d> corners{ polygon[triangle[0]],
                                          polygon[triangle[1]],
                                          polygon[triangle[2]] };
      const double area{ jcad::geom::SignedArea(corners) };
      EXPECT_GT(area, 0.0);
      total += area;
    }
    return total;
  }

} // namespace

TEST(Triangulate, SquareGivesTwoTriangles)
{
  const std::vector<Point2d> square{ { 0.0, 0.0 }, { 1.0, 0.0 }, { 1.0, 1.0 }, { 0.0, 1.0 } };
  const std::vector<Triangle> triangles{ Triangulate<DoubleKernel>(square) };
  EXPECT_EQ(triangles.size(), 2U);
  EXPECT_DOUBLE_EQ(TotalArea(square, triangles), 1.0);
}

TEST(Triangulate, ConcavePolygonCoversItsArea)
{
  const std::vector<Point2d> notch{ { 5.9, -5.0 },
                                    { 3.0, -2.364 },
                                    { 3.0, -3.0 },
                                    { 0.0, -3.0 },
                                    { 0.0, -5.0 } };
  const std::vector<Triangle> triangles{ Triangulate<DoubleKernel>(notch) };
  EXPECT_EQ(triangles.size(), 3U);
  EXPECT_NEAR(TotalArea(notch, triangles), jcad::geom::SignedArea(notch), 1e-12);
}

TEST(Triangulate, SpikeAndCollinearPointsAreSkipped)
{
  // A square with a dangling edge from (1, 0.5) to (0.5, 0.5) and a collinear point.
  const std::vector<Point2d> polygon{ { 0.0, 0.0 }, { 0.5, 0.0 }, { 1.0, 0.0 }, { 1.0, 0.5 },
                                      { 0.5, 0.5 }, { 1.0, 0.5 }, { 1.0, 1.0 }, { 0.0, 1.0 } };
  const std::vector<Triangle> triangles{ Triangulate<DoubleKernel>(polygon) };
  EXPECT_NEAR(TotalArea(polygon, triangles), 1.0, 1e-12);
}
