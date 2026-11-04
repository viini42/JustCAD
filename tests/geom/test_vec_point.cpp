#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "justcad/geom/point2.hpp"

using namespace justcad::geom;
using P = Point2<double>;
using V = Vec2<double>;

TEST_CASE("Operações afins entre pontos e vetores") {
    constexpr P a{1.0, 2.0};
    constexpr P b{4.0, 6.0};

    STATIC_REQUIRE(b - a == V{3.0, 4.0});
    STATIC_REQUIRE(a + V{3.0, 4.0} == b);
    STATIC_REQUIRE(lerp(a, b, 0.5) == P{2.5, 4.0});
    REQUIRE(length(b - a) == Catch::Approx(5.0));
    REQUIRE(squaredDistance(a, b) == Catch::Approx(25.0));
}

TEST_CASE("Produto escalar, vetorial e perpendicular") {
    constexpr V x{1.0, 0.0};
    constexpr V y{0.0, 1.0};

    STATIC_REQUIRE(dot(x, y) == 0.0);
    STATIC_REQUIRE(cross(x, y) == 1.0);
    STATIC_REQUIRE(cross(y, x) == -1.0);
    STATIC_REQUIRE(perp(x) == y);
}
