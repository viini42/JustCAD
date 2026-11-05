#include "euler/euler_op.hpp"
#include "topo/half_edge.hpp"
#include "topo/topology.hpp"

#include <cstddef>
#include <gtest/gtest.h>
#include <optional>
#include <random>
#include <vector>

using jcad::euler::Apply;
using jcad::euler::Corner;
using jcad::euler::EulerOp;
using jcad::euler::JoinEdgeKillVertex;
using jcad::euler::KillEdgeFace;
using jcad::euler::KillEdgeMakeRing;
using jcad::euler::KillEdgeVertex;
using jcad::euler::KillVertexRing;
using jcad::euler::LoopOf;
using jcad::euler::MakeEdgeFace;
using jcad::euler::MakeEdgeKillRing;
using jcad::euler::MakeEdgeVertex;
using jcad::euler::MakeVertexRing;
using jcad::euler::MoveRing;
using jcad::euler::SplitEdgeMakeVertex;
using jcad::topo::CurveId;
using jcad::topo::FaceId;
using jcad::topo::GeomPointId;
using jcad::topo::HalfEdgeId;
using jcad::topo::LoopId;
using jcad::topo::Topology;
using jcad::topo::Twin;
using jcad::topo::VertexId;

namespace
{

  void ExpectValid(const Topology& topology)
  {
    const auto result{ topology.Validate() };
    EXPECT_TRUE(result.has_value()) << result.error();
  }

  /** Applies `op`, checks the result, then checks that its inverse restores the start. */
  EulerOp ApplyAndCheckInverse(Topology& topology, const EulerOp& op)
  {
    const Topology before{ topology };
    const EulerOp inverse{ Apply(topology, op) };
    ExpectValid(topology);
    Topology undone{ topology };
    const EulerOp redo{ Apply(undone, inverse) };
    ExpectValid(undone);
    EXPECT_EQ(undone, before);
    Apply(undone, redo);
    EXPECT_EQ(undone, topology);
    return inverse;
  }

  Corner CornerOf(const Topology& topology, VertexId vertex)
  {
    return Corner{ vertex, topology.VertexAt(vertex).outgoing };
  }

  /** A path v0 - v1 - v2 floating in the unbounded face. */
  class EulerOpTest : public ::testing::Test
  {
  protected:
    VertexId AddVertex(std::uint32_t point)
    {
      const KillVertexRing kill{ Apply(
        m_topology,
        MakeVertexRing{ m_topology.UnboundedFace(), GeomPointId{ point }, {}, {} }) };
      return kill.vertex;
    }

    HalfEdgeId AddDangling(VertexId from, std::uint32_t point)
    {
      return Apply(m_topology,
                   MakeEdgeVertex{ CornerOf(m_topology, from), GeomPointId{ point }, {}, {}, {} })
        .half_edge;
    }

    void MakePath()
    {
      m_v0 = AddVertex(0);
      m_e01 = AddDangling(m_v0, 1);
      m_v1 = m_topology.Target(m_e01);
      m_e12 = AddDangling(m_v1, 2);
      m_v2 = m_topology.Target(m_e12);
    }

    Topology m_topology;
    VertexId m_v0;
    VertexId m_v1;
    VertexId m_v2;
    HalfEdgeId m_e01;
    HalfEdgeId m_e12;
  };

} // namespace

TEST_F(EulerOpTest, MakeVertexRingAddsInnerLoopToFace)
{
  const EulerOp op{ MakeVertexRing{ m_topology.UnboundedFace(), GeomPointId{ 0 }, {}, {} } };
  ApplyAndCheckInverse(m_topology, op);
  EXPECT_EQ(m_topology.VertexCount(), 1U);
  EXPECT_EQ(m_topology.InnerLoopCount(), 1U);
}

TEST_F(EulerOpTest, MakeEdgeVertexFromIsolatedVertexMakesSegment)
{
  const VertexId v0{ AddVertex(0) };
  const EulerOp op{ MakeEdgeVertex{ CornerOf(m_topology, v0), GeomPointId{ 1 }, {}, {}, {} } };
  ApplyAndCheckInverse(m_topology, op);
  EXPECT_EQ(m_topology.EdgeCount(), 1U);
  EXPECT_EQ(m_topology.LoopCount(), 1U);
  EXPECT_FALSE(m_topology.VertexAt(v0).ring.Valid());
}

TEST_F(EulerOpTest, MakeEdgeVertexFromInnerVertexExtendsLoop)
{
  MakePath();
  EXPECT_EQ(m_topology.EdgeCount(), 2U);
  EXPECT_EQ(m_topology.LoopHalfEdges(m_topology.HalfEdgeAt(m_e01).loop).size(), 4U);
  ApplyAndCheckInverse(m_topology,
                       MakeEdgeVertex{ CornerOf(m_topology, m_v1), GeomPointId{ 3 }, {}, {}, {} });
  EXPECT_EQ(m_topology.OutgoingHalfEdges(m_v1).size(), 3U);
}

TEST_F(EulerOpTest, KillEdgeVertexOfLastEdgeLeavesIsolatedVertex)
{
  const VertexId v0{ AddVertex(0) };
  const HalfEdgeId edge{ AddDangling(v0, 1) };
  ApplyAndCheckInverse(m_topology, KillEdgeVertex{ edge });
  EXPECT_EQ(m_topology.VertexCount(), 1U);
  EXPECT_TRUE(m_topology.VertexAt(v0).ring.Valid());
}

TEST_F(EulerOpTest, MakeEdgeFaceClosesTriangle)
{
  MakePath();
  const EulerOp op{
    MakeEdgeFace{ CornerOf(m_topology, m_v2), CornerOf(m_topology, m_v0), {}, {}, {}, {} }
  };
  const EulerOp inverse{ ApplyAndCheckInverse(m_topology, op) };
  EXPECT_EQ(m_topology.FaceCount(), 2U);
  const HalfEdgeId closing{ std::get<KillEdgeFace>(inverse).half_edge };
  const LoopId inside{ m_topology.HalfEdgeAt(closing).loop };
  EXPECT_TRUE(m_topology.IsOuterLoop(inside));
  EXPECT_EQ(m_topology.LoopHalfEdges(inside).size(), 3U);
  EXPECT_FALSE(m_topology.IsOuterLoop(m_topology.HalfEdgeAt(Twin(closing)).loop));
}

TEST_F(EulerOpTest, KillEdgeFaceRemovesTriangleFace)
{
  MakePath();
  const KillEdgeFace kill{ Apply(
    m_topology,
    MakeEdgeFace{ CornerOf(m_topology, m_v2), CornerOf(m_topology, m_v0), {}, {}, {}, {} }) };
  ApplyAndCheckInverse(m_topology, kill);
  EXPECT_EQ(m_topology.FaceCount(), 1U);
  EXPECT_EQ(m_topology.EdgeCount(), 2U);
}

TEST_F(EulerOpTest, KillEdgeMakeRingOnSegmentLeavesTwoVertices)
{
  const VertexId v0{ AddVertex(0) };
  const HalfEdgeId edge{ AddDangling(v0, 1) };
  ApplyAndCheckInverse(m_topology, KillEdgeMakeRing{ edge, {} });
  EXPECT_EQ(m_topology.EdgeCount(), 0U);
  EXPECT_EQ(m_topology.InnerLoopCount(), 2U);
}

TEST_F(EulerOpTest, KillEdgeMakeRingOnMiddleOfPathSplitsComponents)
{
  MakePath();
  ApplyAndCheckInverse(m_topology, KillEdgeMakeRing{ m_e12, {} });
  EXPECT_EQ(m_topology.InnerLoopCount(), 2U);
  EXPECT_TRUE(m_topology.VertexAt(m_v2).ring.Valid());
  EXPECT_FALSE(m_topology.VertexAt(m_v1).ring.Valid());
}

TEST_F(EulerOpTest, MakeEdgeKillRingJoinsTwoSegments)
{
  MakePath();
  const VertexId far{ AddVertex(5) };
  static_cast<void>(AddDangling(far, 6));
  ASSERT_EQ(m_topology.InnerLoopCount(), 2U);
  ApplyAndCheckInverse(
    m_topology,
    MakeEdgeKillRing{ CornerOf(m_topology, m_v1), CornerOf(m_topology, far), {}, {} });
  EXPECT_EQ(m_topology.InnerLoopCount(), 1U);
}

TEST_F(EulerOpTest, MakeEdgeKillRingIntoOuterLoopKeepsOuterLoop)
{
  MakePath();
  const KillEdgeFace kill{ Apply(
    m_topology,
    MakeEdgeFace{ CornerOf(m_topology, m_v2), CornerOf(m_topology, m_v0), {}, {}, {}, {} }) };
  const LoopId outer{ m_topology.HalfEdgeAt(kill.half_edge).loop };
  const FaceId triangle{ m_topology.LoopAt(outer).face };
  const VertexId inside{ AddVertex(9) };
  Apply(m_topology, MoveRing{ m_topology.VertexAt(inside).ring, triangle });
  HalfEdgeId at_v0{};
  for (const HalfEdgeId half_edge : m_topology.LoopHalfEdges(outer))
  {
    if (m_topology.HalfEdgeAt(half_edge).origin == m_v0)
      at_v0 = half_edge;
  }
  ApplyAndCheckInverse(
    m_topology,
    MakeEdgeKillRing{ CornerOf(m_topology, inside), Corner{ m_v0, at_v0 }, {}, {} });
  EXPECT_TRUE(m_topology.Contains(outer));
  EXPECT_TRUE(m_topology.IsOuterLoop(outer));
  EXPECT_TRUE(m_topology.FaceAt(triangle).inner.empty());
}

TEST_F(EulerOpTest, SplitEdgeMakeVertexInsertsVertex)
{
  MakePath();
  const EulerOp op{
    SplitEdgeMakeVertex{ m_e01, GeomPointId{ 7 }, CurveId{ 1 }, CurveId{ 2 }, {}, {} }
  };
  ApplyAndCheckInverse(m_topology, op);
  EXPECT_EQ(m_topology.VertexCount(), 4U);
  EXPECT_EQ(m_topology.EdgeCount(), 3U);
  const VertexId middle{ m_topology.Target(m_e01) };
  EXPECT_EQ(m_topology.VertexAt(middle).point, GeomPointId{ 7 });
  EXPECT_EQ(m_topology.EdgeAt(jcad::topo::EdgeOf(m_e01)).curve, CurveId{ 1 });
}

TEST_F(EulerOpTest, SplitEdgeMakeVertexAtDanglingEndKeepsHair)
{
  MakePath();
  ApplyAndCheckInverse(m_topology, SplitEdgeMakeVertex{ m_e12, GeomPointId{ 7 }, {}, {}, {}, {} });
  EXPECT_EQ(m_topology.LoopHalfEdges(m_topology.HalfEdgeAt(m_e12).loop).size(), 6U);
}

TEST_F(EulerOpTest, JoinEdgeKillVertexRemovesMiddleVertex)
{
  MakePath();
  ApplyAndCheckInverse(m_topology, JoinEdgeKillVertex{ m_e01, CurveId{ 5 } });
  EXPECT_EQ(m_topology.VertexCount(), 2U);
  EXPECT_EQ(m_topology.Target(m_e01), m_v2);
}

TEST_F(EulerOpTest, MoveRingIntoTriangleFace)
{
  MakePath();
  const KillEdgeFace kill{ Apply(
    m_topology,
    MakeEdgeFace{ CornerOf(m_topology, m_v2), CornerOf(m_topology, m_v0), {}, {}, {}, {} }) };
  const FaceId triangle{ m_topology.LoopAt(m_topology.HalfEdgeAt(kill.half_edge).loop).face };
  const VertexId inside{ AddVertex(9) };
  ApplyAndCheckInverse(m_topology, MoveRing{ m_topology.VertexAt(inside).ring, triangle });
  EXPECT_EQ(m_topology.FaceAt(triangle).inner.size(), 1U);
}

TEST_F(EulerOpTest, UndoAllOperatorsRestoresEmptyTopology)
{
  std::vector<EulerOp> undo;
  undo.push_back(
    Apply(m_topology, MakeVertexRing{ m_topology.UnboundedFace(), GeomPointId{ 0 }, {}, {} }));
  const VertexId v0{ std::get<KillVertexRing>(undo.back()).vertex };
  undo.push_back(
    Apply(m_topology, MakeEdgeVertex{ CornerOf(m_topology, v0), GeomPointId{ 1 }, {}, {}, {} }));
  const VertexId v1{ m_topology.Target(std::get<KillEdgeVertex>(undo.back()).half_edge) };
  undo.push_back(
    Apply(m_topology, MakeEdgeVertex{ CornerOf(m_topology, v1), GeomPointId{ 2 }, {}, {}, {} }));
  const VertexId v2{ m_topology.Target(std::get<KillEdgeVertex>(undo.back()).half_edge) };
  undo.push_back(
    Apply(m_topology,
          MakeEdgeFace{ CornerOf(m_topology, v2), CornerOf(m_topology, v0), {}, {}, {}, {} }));
  ExpectValid(m_topology);
  for (auto op{ undo.rbegin() }; op != undo.rend(); ++op)
    Apply(m_topology, *op);
  EXPECT_EQ(m_topology, Topology{});
}

namespace
{

  /**
   * Picks a random operator whose preconditions hold. The topology need not be
   * geometrically realizable: the operators are purely combinatorial.
   */
  class RandomOps
  {
  public:
    explicit RandomOps(unsigned seed) : m_random{ seed } {}

    std::optional<EulerOp> Pick(const Topology& topology)
    {
      switch (Index(11))
      {
      case 0:
        return MakeVertexRing{ Choose(topology.FaceIds()), GeomPointId{ m_point++ }, {}, {} };
      case 1:
        return PickMakeEdgeVertex(topology);
      case 2:
        return PickMakeEdgeFace(topology);
      case 3:
        return PickMakeEdgeKillRing(topology);
      case 4:
        return PickKillVertexRing(topology);
      case 5:
        return PickKillEdge(
          topology,
          [&](HalfEdgeId h) { return CanKillEdgeVertex(topology, h); },
          false);
      case 6:
        return PickKillEdge(
          topology,
          [&](HalfEdgeId h) { return CanKillEdgeFace(topology, h); },
          true);
      case 7:
        return PickKillEdge(topology, [&](HalfEdgeId h) { return SameLoop(topology, h); }, false);
      case 8:
        return PickSplitEdge(topology);
      case 9:
        return PickJoinEdges(topology);
      default:
        return PickMoveRing(topology);
      }
    }

  private:
    std::size_t Index(std::size_t size)
    {
      return std::uniform_int_distribution<std::size_t>{ 0, size - 1 }(m_random);
    }

    template <typename T>
    T Choose(const std::vector<T>& items)
    {
      return items[Index(items.size())];
    }

    Corner RandomCorner(const Topology& topology, VertexId vertex)
    {
      const std::vector<HalfEdgeId> outgoing{ topology.OutgoingHalfEdges(vertex) };
      return Corner{ vertex, outgoing.empty() ? HalfEdgeId{} : Choose(outgoing) };
    }

    std::vector<HalfEdgeId> AllHalfEdges(const Topology& topology)
    {
      std::vector<HalfEdgeId> half_edges;
      for (const jcad::topo::EdgeId edge : topology.EdgeIds())
      {
        half_edges.push_back(jcad::topo::FirstHalfEdge(edge));
        half_edges.push_back(jcad::topo::SecondHalfEdge(edge));
      }
      return half_edges;
    }

    static bool SameLoop(const Topology& topology, HalfEdgeId halfEdge)
    {
      return topology.HalfEdgeAt(halfEdge).loop == topology.HalfEdgeAt(Twin(halfEdge)).loop;
    }

    static bool CanKillEdgeVertex(const Topology& topology, HalfEdgeId halfEdge)
    {
      return topology.HalfEdgeAt(halfEdge).next == Twin(halfEdge);
    }

    static bool CanKillEdgeFace(const Topology& topology, HalfEdgeId halfEdge)
    {
      const LoopId loop{ topology.HalfEdgeAt(halfEdge).loop };
      return !SameLoop(topology, halfEdge) && topology.IsOuterLoop(loop) &&
             topology.FaceAt(topology.LoopAt(loop).face).inner.empty();
    }

    std::optional<EulerOp> PickMakeEdgeVertex(const Topology& topology)
    {
      if (topology.VertexCount() == 0)
        return std::nullopt;
      const Corner from{ RandomCorner(topology, Choose(topology.VertexIds())) };
      return MakeEdgeVertex{ from, GeomPointId{ m_point++ }, {}, {}, {} };
    }

    std::optional<EulerOp> PickMakeEdgeFace(const Topology& topology)
    {
      if (topology.EdgeCount() == 0)
        return std::nullopt;
      const std::vector<HalfEdgeId> loop{ topology.LoopHalfEdges(
        topology.HalfEdgeAt(Choose(AllHalfEdges(topology))).loop) };
      const HalfEdgeId a{ Choose(loop) };
      const HalfEdgeId b{ Choose(loop) };
      const VertexId va{ topology.HalfEdgeAt(a).origin };
      const VertexId vb{ topology.HalfEdgeAt(b).origin };
      if (va == vb)
        return std::nullopt;
      return MakeEdgeFace{ Corner{ va, a }, Corner{ vb, b }, {}, {}, {}, {} };
    }

    std::optional<EulerOp> PickMakeEdgeKillRing(const Topology& topology)
    {
      if (topology.VertexCount() < 2)
        return std::nullopt;
      const Corner from{ RandomCorner(topology, Choose(topology.VertexIds())) };
      const Corner to{ RandomCorner(topology, Choose(topology.VertexIds())) };
      const LoopId from_loop{ LoopOf(topology, from) };
      const LoopId to_loop{ LoopOf(topology, to) };
      if (from.vertex == to.vertex || from_loop == to_loop)
        return std::nullopt;
      if (topology.LoopAt(from_loop).face != topology.LoopAt(to_loop).face)
        return std::nullopt;
      return MakeEdgeKillRing{ from, to, {}, {} };
    }

    std::optional<EulerOp> PickKillVertexRing(const Topology& topology)
    {
      std::vector<VertexId> isolated;
      for (const VertexId vertex : topology.VertexIds())
      {
        const LoopId ring{ topology.VertexAt(vertex).ring };
        if (ring.Valid() && !topology.IsOuterLoop(ring))
          isolated.push_back(vertex);
      }
      if (isolated.empty())
        return std::nullopt;
      return KillVertexRing{ Choose(isolated) };
    }

    template <typename Predicate>
    std::optional<EulerOp> PickKillEdge(const Topology& topology, Predicate predicate, bool face)
    {
      std::vector<HalfEdgeId> candidates;
      for (const HalfEdgeId half_edge : AllHalfEdges(topology))
      {
        if (predicate(half_edge))
          candidates.push_back(half_edge);
      }
      if (candidates.empty())
        return std::nullopt;
      const HalfEdgeId chosen{ Choose(candidates) };
      if (face)
        return KillEdgeFace{ chosen };
      if (CanKillEdgeVertex(topology, chosen) && Index(2) == 0)
        return KillEdgeVertex{ chosen };
      if (SameLoop(topology, chosen))
        return KillEdgeMakeRing{ chosen, {} };
      return std::nullopt;
    }

    std::optional<EulerOp> PickSplitEdge(const Topology& topology)
    {
      if (topology.EdgeCount() == 0)
        return std::nullopt;
      const HalfEdgeId half_edge{ Choose(AllHalfEdges(topology)) };
      return SplitEdgeMakeVertex{
        half_edge, GeomPointId{ m_point++ }, CurveId{ m_point }, CurveId{ m_point + 1 }, {}, {}
      };
    }

    static bool CanJoin(const Topology& topology, HalfEdgeId halfEdge)
    {
      const HalfEdgeId joined{ topology.HalfEdgeAt(halfEdge).next };
      if (joined == Twin(halfEdge) || topology.HalfEdgeAt(Twin(joined)).next != Twin(halfEdge))
        return false;
      return topology.Target(joined) != topology.HalfEdgeAt(halfEdge).origin;
    }

    std::optional<EulerOp> PickJoinEdges(const Topology& topology)
    {
      std::vector<HalfEdgeId> candidates;
      for (const HalfEdgeId half_edge : AllHalfEdges(topology))
      {
        if (CanJoin(topology, half_edge))
          candidates.push_back(half_edge);
      }
      if (candidates.empty())
        return std::nullopt;
      return JoinEdgeKillVertex{ Choose(candidates), CurveId{ m_point++ } };
    }

    std::optional<EulerOp> PickMoveRing(const Topology& topology)
    {
      std::vector<LoopId> inner;
      for (const LoopId loop : topology.LoopIds())
      {
        if (!topology.IsOuterLoop(loop))
          inner.push_back(loop);
      }
      if (inner.empty())
        return std::nullopt;
      return MoveRing{ Choose(inner), Choose(topology.FaceIds()) };
    }

    std::mt19937 m_random;
    std::uint32_t m_point{ 0 };
  };

} // namespace

TEST(EulerOp, ApplyRandomSequenceKeepsInvariantsAndUndoesToStart)
{
  for (unsigned seed{ 1 }; seed <= 20; ++seed)
  {
    RandomOps random{ seed };
    Topology topology;
    std::vector<Topology> history;
    std::vector<EulerOp> undo;
    for (int step{ 0 }; step < 200; ++step)
    {
      const std::optional<EulerOp> op{ random.Pick(topology) };
      if (!op)
        continue;
      history.push_back(topology);
      undo.push_back(Apply(topology, *op));
      ASSERT_TRUE(topology.Validate().has_value())
        << "seed " << seed << ": " << topology.Validate().error();
    }
    while (!undo.empty())
    {
      Apply(topology, undo.back());
      undo.pop_back();
      ASSERT_EQ(topology, history.back()) << "seed " << seed;
      history.pop_back();
    }
  }
}
