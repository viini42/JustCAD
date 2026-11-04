#include "justcad/geom/vec2.hpp"

#include <gtest/gtest.h>

using Vec2d = justcad::geom::Vec2<double>;

TEST(Vec2, ArithmeticOperatorsActComponentWise) {
    constexpr Vec2d a{1.0, 2.0};
    constexpr Vec2d b{3.0, -1.0};

    static_assert(a + b == Vec2d{4.0, 1.0});
    static_assert(a - b == Vec2d{-2.0, 3.0});
    static_assert(-a == Vec2d{-1.0, -2.0});
    static_assert(a * 2.0 == Vec2d{2.0, 4.0});
    static_assert(2.0 * a == Vec2d{2.0, 4.0});
    static_assert(a / 2.0 == Vec2d{0.5, 1.0});
    SUCCEED();
}

TEST(Vec2, DotPerpendicularVectorsReturnsZero) {
    EXPECT_EQ(justcad::geom::Dot(Vec2d{1.0, 0.0}, Vec2d{0.0, 1.0}), 0.0);
}

TEST(Vec2, CrossCounterClockwisePairReturnsPositive) {
    EXPECT_EQ(justcad::geom::Cross(Vec2d{1.0, 0.0}, Vec2d{0.0, 1.0}), 1.0);
    EXPECT_EQ(justcad::geom::Cross(Vec2d{0.0, 1.0}, Vec2d{1.0, 0.0}), -1.0);
}

TEST(Vec2, LengthThreeFourVectorReturnsFive) {
    EXPECT_DOUBLE_EQ(justcad::geom::Length(Vec2d{3.0, 4.0}), 5.0);
    EXPECT_DOUBLE_EQ(justcad::geom::SquaredLength(Vec2d{3.0, 4.0}), 25.0);
}

TEST(Vec2, PerpUnitXReturnsUnitY) {
    EXPECT_EQ(justcad::geom::Perp(Vec2d{1.0, 0.0}), (Vec2d{0.0, 1.0}));
}
