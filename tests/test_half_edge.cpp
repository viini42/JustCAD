#include "topo/half_edge.hpp"

#include <gtest/gtest.h>

using jcad::topo::EdgeId;
using jcad::topo::EdgeOf;
using jcad::topo::FirstHalfEdge;
using jcad::topo::HalfEdgeId;
using jcad::topo::SecondHalfEdge;
using jcad::topo::Twin;

TEST(HalfEdge, TwinOfTwinIsIdentity)
{
  static_assert(Twin(Twin(HalfEdgeId{ 7 })) == HalfEdgeId{ 7 });
  static_assert(Twin(HalfEdgeId{ 6 }) == HalfEdgeId{ 7 });
  SUCCEED();
}

TEST(HalfEdge, BothHalfEdgesBelongToTheirEdge)
{
  static_assert(EdgeOf(FirstHalfEdge(EdgeId{ 3 })) == EdgeId{ 3 });
  static_assert(EdgeOf(SecondHalfEdge(EdgeId{ 3 })) == EdgeId{ 3 });
  static_assert(Twin(FirstHalfEdge(EdgeId{ 3 })) == SecondHalfEdge(EdgeId{ 3 }));
  SUCCEED();
}
