#include "justcad/topo/id.hpp"

#include <gtest/gtest.h>

#include <type_traits>
#include <unordered_set>

using justcad::topo::EdgeId;
using justcad::topo::FaceId;
using justcad::topo::VertexId;

TEST(Id, DefaultConstructedIsInvalid) {
    static_assert(!VertexId{}.Valid());
    static_assert(VertexId{0}.Valid());
    SUCCEED();
}

TEST(Id, DifferentTagsAreNotConvertible) {
    static_assert(!std::is_convertible_v<VertexId, EdgeId>);
    static_assert(!std::is_convertible_v<std::uint32_t, VertexId>);
    SUCCEED();
}

TEST(Id, LessThanComparesValues) {
    static_assert(EdgeId{1} < EdgeId{2});
    SUCCEED();
}

TEST(Id, HashDuplicateIdsCollapseInSet) {
    const std::unordered_set<FaceId> faces{FaceId{1}, FaceId{2}, FaceId{1}};
    EXPECT_EQ(faces.size(), 2U);
}
