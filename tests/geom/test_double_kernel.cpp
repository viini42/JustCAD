#include <catch2/catch_test_macros.hpp>

#include "justcad/geom/double_kernel.hpp"

using namespace justcad::geom;
using K = DoubleKernel;
using P = K::Point2;

TEST_CASE("orientation distingue esquerda, direita e colinear") {
    const P a{0.0, 0.0};
    const P b{1.0, 0.0};

    REQUIRE(K::orientation(a, b, P{0.5, 1.0}) == Sign::Positive);
    REQUIRE(K::orientation(a, b, P{0.5, -1.0}) == Sign::Negative);
    REQUIRE(K::orientation(a, b, P{5.0, 0.0}) == Sign::Zero);
    REQUIRE(K::orientation(a, a, P{5.0, 3.0}) == Sign::Zero);  // degenerado
}

TEST_CASE("A tolerância é uma distância, independente da escala do segmento") {
    const ScopedTolerance tol{1e-3};
    // c está a 0.5e-3 da reta: colinear tanto para um segmento curto quanto longo.
    REQUIRE(K::orientation(P{0.0, 0.0}, P{1.0, 0.0}, P{0.5, 0.5e-3}) == Sign::Zero);
    REQUIRE(K::orientation(P{0.0, 0.0}, P{1e6, 0.0}, P{0.5, 0.5e-3}) == Sign::Zero);
    REQUIRE(K::orientation(P{0.0, 0.0}, P{1e6, 0.0}, P{0.5, 2e-3}) == Sign::Positive);
}

TEST_CASE("equal e compareXY respeitam a tolerância") {
    const ScopedTolerance tol{1e-6};
    REQUIRE(K::equal(P{1.0, 1.0}, P{1.0 + 5e-7, 1.0}));
    REQUIRE_FALSE(K::equal(P{1.0, 1.0}, P{1.0 + 2e-6, 1.0}));

    REQUIRE(K::compareXY(P{0.0, 0.0}, P{1.0, -5.0}) == Sign::Negative);
    REQUIRE(K::compareXY(P{1.0, 2.0}, P{1.0, 1.0}) == Sign::Positive);
    REQUIRE(K::compareXY(P{1.0, 1.0}, P{1.0, 1.0 + 1e-7}) == Sign::Zero);
}

TEST_CASE("onSegment inclui as extremidades e rejeita pontos fora") {
    const P a{0.0, 0.0};
    const P b{2.0, 2.0};

    REQUIRE(K::onSegment(a, b, P{1.0, 1.0}));
    REQUIRE(K::onSegment(a, b, a));
    REQUIRE(K::onSegment(a, b, b));
    REQUIRE_FALSE(K::onSegment(a, b, P{3.0, 3.0}));   // colinear, mas fora
    REQUIRE_FALSE(K::onSegment(a, b, P{1.0, 1.5}));   // fora da reta
}

TEST_CASE("ScopedTolerance restaura o valor anterior") {
    const double before = K::tolerance();
    {
        const ScopedTolerance tol{0.5};
        REQUIRE(K::tolerance() == 0.5);
    }
    REQUIRE(K::tolerance() == before);
}
