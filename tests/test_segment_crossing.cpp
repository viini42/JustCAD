#include "geom/double_kernel.hpp"
#include "geom/segment_crossing.hpp"

#include <gtest/gtest.h>

using jcad::geom::DoubleKernel;
using jcad::geom::ProperCrossing;
using Point2d = DoubleKernel::Point2;

TEST(SegmentCrossing, ProperCrossingOfDiagonalsReturnsCenter)
{
  const auto crossing{ ProperCrossing<DoubleKernel>(Point2d{ 0.0, 0.0 },
                                                    Point2d{ 2.0, 2.0 },
                                                    Point2d{ 0.0, 2.0 },
                                                    Point2d{ 2.0, 0.0 }) };
  ASSERT_TRUE(crossing.has_value());
  EXPECT_TRUE(DoubleKernel::Equal(*crossing, Point2d{ 1.0, 1.0 }));
}

TEST(SegmentCrossing, ProperCrossingTouchingAtEndReturnsNothing)
{
  EXPECT_FALSE(ProperCrossing<DoubleKernel>(Point2d{ 0.0, 0.0 },
                                            Point2d{ 2.0, 0.0 },
                                            Point2d{ 1.0, 0.0 },
                                            Point2d{ 1.0, 1.0 })
                 .has_value());
}

TEST(SegmentCrossing, ProperCrossingDisjointOrParallelReturnsNothing)
{
  EXPECT_FALSE(ProperCrossing<DoubleKernel>(Point2d{ 0.0, 0.0 },
                                            Point2d{ 1.0, 0.0 },
                                            Point2d{ 2.0, -1.0 },
                                            Point2d{ 2.0, 1.0 })
                 .has_value());
  EXPECT_FALSE(ProperCrossing<DoubleKernel>(Point2d{ 0.0, 0.0 },
                                            Point2d{ 2.0, 0.0 },
                                            Point2d{ 1.0, 0.0 },
                                            Point2d{ 3.0, 0.0 })
                 .has_value());
}
