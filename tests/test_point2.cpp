#include "justcad/geom/point2.hpp"

#include <gtest/gtest.h>

using Point2d = justcad::geom::Point2<double>;
using Vec2d = justcad::geom::Vec2<double>;

TEST(Point2, SubtractPointsReturnsVector) {
    EXPECT_EQ((Point2d{4.0, 6.0} - Point2d{1.0, 2.0}), (Vec2d{3.0, 4.0}));
}

TEST(Point2, AddVectorReturnsTranslatedPoint) {
    EXPECT_EQ((Point2d{1.0, 2.0} + Vec2d{3.0, 4.0}), (Point2d{4.0, 6.0}));
}

TEST(Point2, LerpHalfReturnsMidpoint) {
    EXPECT_EQ(justcad::geom::Lerp(Point2d{1.0, 2.0}, Point2d{4.0, 6.0}, 0.5), (Point2d{2.5, 4.0}));
}

TEST(Point2, SquaredDistanceThreeFourApartReturnsTwentyFive) {
    EXPECT_DOUBLE_EQ(justcad::geom::SquaredDistance(Point2d{1.0, 2.0}, Point2d{4.0, 6.0}), 25.0);
}
