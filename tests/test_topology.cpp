#include "topo/topology.hpp"

#include <gtest/gtest.h>

using jcad::topo::Edge;
using jcad::topo::EdgeId;
using jcad::topo::Face;
using jcad::topo::FirstHalfEdge;
using jcad::topo::GeomPointId;
using jcad::topo::HalfEdgeId;
using jcad::topo::Loop;
using jcad::topo::LoopId;
using jcad::topo::SecondHalfEdge;
using jcad::topo::Topology;
using jcad::topo::Vertex;
using jcad::topo::VertexId;

namespace
{

  /** Builds, by hand, a single segment v0 -> v1 floating in the unbounded face. */
  struct Segment
  {
    Topology topology;
    VertexId v0;
    VertexId v1;
    EdgeId edge;
    LoopId loop;
  };

  Segment MakeSegment()
  {
    Segment s{};
    s.edge = s.topology.AddEdge(Edge{});
    const HalfEdgeId out{ FirstHalfEdge(s.edge) };
    const HalfEdgeId back{ SecondHalfEdge(s.edge) };
    s.v0 = s.topology.AddVertex(Vertex{ out, {}, GeomPointId{ 0 } });
    s.v1 = s.topology.AddVertex(Vertex{ back, {}, GeomPointId{ 1 } });
    s.loop = s.topology.AddLoop(Loop{ out, {}, s.topology.UnboundedFace() });
    s.topology.AttachInnerLoop(s.topology.UnboundedFace(), s.loop);
    s.topology.HalfEdgeAt(out) = { s.v0, back, back, s.loop };
    s.topology.HalfEdgeAt(back) = { s.v1, out, out, s.loop };
    return s;
  }

} // namespace

TEST(Topology, ConstructorCreatesOnlyUnboundedFace)
{
  const Topology topology;
  EXPECT_EQ(topology.FaceCount(), 1U);
  EXPECT_EQ(topology.VertexCount(), 0U);
  EXPECT_TRUE(topology.Contains(topology.UnboundedFace()));
  EXPECT_TRUE(topology.Validate().has_value());
}

TEST(Topology, ValidateHandBuiltSegmentSucceeds)
{
  const Segment s{ MakeSegment() };
  EXPECT_TRUE(s.topology.Validate().has_value()) << s.topology.Validate().error();
  EXPECT_EQ(s.topology.Target(FirstHalfEdge(s.edge)), s.v1);
  EXPECT_EQ(s.topology.LoopHalfEdges(s.loop).size(), 2U);
  EXPECT_EQ(s.topology.OutgoingHalfEdges(s.v0).size(), 1U);
  EXPECT_FALSE(s.topology.IsOuterLoop(s.loop));
}

TEST(Topology, ValidateBrokenNextPrevFails)
{
  Segment s{ MakeSegment() };
  s.topology.HalfEdgeAt(FirstHalfEdge(s.edge)).prev = FirstHalfEdge(s.edge);
  EXPECT_FALSE(s.topology.Validate().has_value());
}

TEST(Topology, ValidateHalfEdgeInWrongLoopFails)
{
  Segment s{ MakeSegment() };
  const LoopId other{ s.topology.AddLoop(Loop{ {}, {}, s.topology.UnboundedFace() }) };
  s.topology.HalfEdgeAt(SecondHalfEdge(s.edge)).loop = other;
  EXPECT_FALSE(s.topology.Validate().has_value());
}

TEST(Topology, ValidateLoopMissingFromFaceFails)
{
  Segment s{ MakeSegment() };
  s.topology.DetachInnerLoop(s.topology.UnboundedFace(), s.loop);
  EXPECT_FALSE(s.topology.Validate().has_value());
}

TEST(Topology, ValidateVertexWithRingAndEdgesFails)
{
  Segment s{ MakeSegment() };
  s.topology.VertexAt(s.v0).ring = s.loop;
  EXPECT_FALSE(s.topology.Validate().has_value());
}

TEST(Topology, ValidateFaceWithBogusOuterLoopFails)
{
  Segment s{ MakeSegment() };
  const LoopId outer{ s.topology.AddLoop(Loop{ {}, s.v0, {} }) };
  const jcad::topo::FaceId face{ s.topology.AddFace(Face{ outer, {} }) };
  s.topology.LoopAt(outer).face = face;
  EXPECT_FALSE(s.topology.Validate().has_value());
}

TEST(Topology, ValidateEdgeWithSameEndsFails)
{
  Segment s{ MakeSegment() };
  s.topology.HalfEdgeAt(SecondHalfEdge(s.edge)).origin = s.v0;
  EXPECT_FALSE(s.topology.Validate().has_value());
}

TEST(Topology, EqualityIgnoresRepresentativeHalfEdges)
{
  const Segment a{ MakeSegment() };
  Segment b{ MakeSegment() };
  EXPECT_EQ(a.topology, b.topology);
  b.topology.LoopAt(b.loop).first = SecondHalfEdge(b.edge);
  EXPECT_EQ(a.topology, b.topology);
  b.topology.VertexAt(b.v0).point = GeomPointId{ 9 };
  EXPECT_NE(a.topology, b.topology);
}

TEST(Topology, AttachInnerLoopKeepsIdsSorted)
{
  Topology topology;
  const jcad::topo::FaceId face{ topology.UnboundedFace() };
  topology.AttachInnerLoop(face, LoopId{ 5 });
  topology.AttachInnerLoop(face, LoopId{ 2 });
  topology.AttachInnerLoop(face, LoopId{ 9 });
  const std::vector<LoopId> expected{ LoopId{ 2 }, LoopId{ 5 }, LoopId{ 9 } };
  EXPECT_EQ(topology.FaceAt(face).inner, expected);
}
