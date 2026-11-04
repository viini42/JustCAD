#include <catch2/catch_test_macros.hpp>

#include <type_traits>
#include <unordered_set>

#include "justcad/topo/ids.hpp"

using namespace justcad::topo;

TEST_CASE("Id padrão é inválido") {
    constexpr VertexId v;
    STATIC_REQUIRE_FALSE(v.valid());
    STATIC_REQUIRE(VertexId{0}.valid());
}

TEST_CASE("Ids de entidades diferentes não se misturam") {
    STATIC_REQUIRE_FALSE(std::is_convertible_v<VertexId, EdgeId>);
    STATIC_REQUIRE_FALSE(std::is_convertible_v<std::uint32_t, VertexId>);
}

TEST_CASE("Ids são ordenáveis e hasheáveis") {
    STATIC_REQUIRE(EdgeId{1} < EdgeId{2});
    std::unordered_set<FaceId> faces{FaceId{1}, FaceId{2}, FaceId{1}};
    REQUIRE(faces.size() == 2);
}
