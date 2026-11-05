#include "geom/bbox.hpp"

#include <gtest/gtest.h>

using BBoxd = jcad::geom::BBox<double>;
using Point2d = jcad::geom::Point2<double>;

TEST(BBox, DefaultConstructedIsEmpty)
{
  const BBoxd box{};
  EXPECT_TRUE(box.Empty());
  EXPECT_FALSE(box.Contains(Point2d{ 0.0, 0.0 }));
}

TEST(BBox, ExpandSinglePointIsNotEmptyWithZeroSize)
{
  BBoxd box{};
  box.Expand(Point2d{ 1.0, 1.0 });
  EXPECT_FALSE(box.Empty());
  EXPECT_EQ(box.Width(), 0.0);
  EXPECT_EQ(box.Height(), 0.0);
}

TEST(BBox, ExpandTwoPointsCoversBoth)
{
  const BBoxd box{ BBoxd::Of(Point2d{ 1.0, 1.0 }, Point2d{ -1.0, 3.0 }) };
  EXPECT_EQ(box.min, (Point2d{ -1.0, 1.0 }));
  EXPECT_EQ(box.max, (Point2d{ 1.0, 3.0 }));
  EXPECT_TRUE(box.Contains(Point2d{ 0.0, 2.0 }));
}

TEST(BBox, IntersectsTouchingCornerReturnsTrue)
{
  const BBoxd a{ BBoxd::Of(Point2d{ 0.0, 0.0 }, Point2d{ 2.0, 2.0 }) };
  EXPECT_TRUE(a.Intersects(BBoxd::Of(Point2d{ 2.0, 2.0 }, Point2d{ 3.0, 3.0 })));
}

TEST(BBox, IntersectsSeparatedBoxReturnsFalseUntilInflated)
{
  const BBoxd a{ BBoxd::Of(Point2d{ 0.0, 0.0 }, Point2d{ 2.0, 2.0 }) };
  const BBoxd c{ BBoxd::Of(Point2d{ 2.1, 0.0 }, Point2d{ 3.0, 1.0 }) };
  EXPECT_FALSE(a.Intersects(c));
  EXPECT_TRUE(a.Inflated(0.2).Intersects(c));
}

TEST(BBox, IntersectsEmptyBoxReturnsFalse)
{
  const BBoxd a{ BBoxd::Of(Point2d{ 0.0, 0.0 }, Point2d{ 2.0, 2.0 }) };
  EXPECT_FALSE(a.Intersects(BBoxd{}));
}
