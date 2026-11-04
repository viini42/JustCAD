#include <catch2/catch_test_macros.hpp>

#include "justcad/geom/bbox.hpp"

using namespace justcad::geom;
using P = Point2<double>;
using B = BBox<double>;

TEST_CASE("BBox começa vazia e cresce com expand") {
    B box;
    REQUIRE(box.empty());
    REQUIRE_FALSE(box.contains(P{0.0, 0.0}));

    box.expand(P{1.0, 1.0});
    REQUIRE_FALSE(box.empty());
    REQUIRE(box.width() == 0.0);

    box.expand(P{-1.0, 3.0});
    REQUIRE(box.min == P{-1.0, 1.0});
    REQUIRE(box.max == P{1.0, 3.0});
    REQUIRE(box.contains(P{0.0, 2.0}));
}

TEST_CASE("Interseção de BBoxes") {
    const B a = B::of(P{0.0, 0.0}, P{2.0, 2.0});
    const B b = B::of(P{2.0, 2.0}, P{3.0, 3.0});  // toca no canto
    const B c = B::of(P{2.1, 0.0}, P{3.0, 1.0});

    REQUIRE(a.intersects(b));
    REQUIRE_FALSE(a.intersects(c));
    REQUIRE(a.inflated(0.2).intersects(c));
    REQUIRE_FALSE(a.intersects(B{}));
}
