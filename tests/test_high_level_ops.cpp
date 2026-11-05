#include "euler/high_level_ops.hpp"
#include "euler/history.hpp"
#include "topo/half_edge.hpp"
#include "topo/topology.hpp"

#include <gtest/gtest.h>

using jcad::euler::AddDanglingEdge;
using jcad::euler::AddedEdge;
using jcad::euler::AddEdge;
using jcad::euler::AddIsolatedVertex;
using jcad::euler::Corner;
using jcad::euler::History;
using jcad::euler::MergeEdges;
using jcad::euler::RemoveEdge;
using jcad::euler::RemoveVertex;
using jcad::euler::SplitCurves;
using jcad::euler::SplitEdge;
using jcad::topo::CurveId;
using jcad::topo::FaceId;
using jcad::topo::GeomPointId;
using jcad::topo::HalfEdgeId;
using jcad::topo::Topology;
using jcad::topo::Twin;
using jcad::topo::VertexId;

namespace
{

  /** Builds a closed square a-b-c-d with high-level operators. */
  class HighLevelOpsTest : public ::testing::Test
  {
  protected:
    Corner CornerOf(VertexId vertex) const
    {
      return Corner{ vertex, m_topology.VertexAt(vertex).outgoing };
    }

    void ExpectValid() const
    {
      const auto result{ m_topology.Validate() };
      EXPECT_TRUE(result.has_value()) << result.error();
    }

    void MakeSquare()
    {
      m_a = AddIsolatedVertex(m_history, m_topology.UnboundedFace(), GeomPointId{ 0 });
      m_ab = AddDanglingEdge(m_history, CornerOf(m_a), GeomPointId{ 1 }, CurveId{ 0 });
      m_b = m_topology.Target(m_ab);
      const HalfEdgeId bc{
        AddDanglingEdge(m_history, CornerOf(m_b), GeomPointId{ 2 }, CurveId{ 1 })
      };
      m_c = m_topology.Target(bc);
      const HalfEdgeId cd{
        AddDanglingEdge(m_history, CornerOf(m_c), GeomPointId{ 3 }, CurveId{ 2 })
      };
      m_d = m_topology.Target(cd);
      m_da = AddEdge(m_history, CornerOf(m_d), CornerOf(m_a), CurveId{ 3 });
    }

    Topology m_topology;
    History m_history{ m_topology };
    VertexId m_a;
    VertexId m_b;
    VertexId m_c;
    VertexId m_d;
    HalfEdgeId m_ab;
    AddedEdge m_da;
  };

} // namespace

TEST_F(HighLevelOpsTest, AddEdgeClosingLoopCreatesFace)
{
  MakeSquare();
  ExpectValid();
  EXPECT_EQ(m_topology.FaceCount(), 2U);
  EXPECT_TRUE(m_da.new_face.Valid());
  EXPECT_EQ(m_topology.HalfEdgeAt(m_da.half_edge).origin, m_d);
  EXPECT_EQ(m_history.UndoCount(), 5U);
}

TEST_F(HighLevelOpsTest, AddEdgeBetweenComponentsJoinsLoops)
{
  const VertexId a{ AddIsolatedVertex(m_history, m_topology.UnboundedFace(), GeomPointId{ 0 }) };
  const VertexId b{ AddIsolatedVertex(m_history, m_topology.UnboundedFace(), GeomPointId{ 1 }) };
  const AddedEdge edge{ AddEdge(m_history, CornerOf(a), CornerOf(b), CurveId{ 0 }) };
  ExpectValid();
  EXPECT_FALSE(edge.new_face.Valid());
  EXPECT_EQ(m_topology.HalfEdgeAt(edge.half_edge).origin, a);
  EXPECT_EQ(m_topology.InnerLoopCount(), 1U);
}

TEST_F(HighLevelOpsTest, AddEdgeFromInnerVertexToOuterLoopReturnsForwardHalfEdge)
{
  MakeSquare();
  const VertexId inside{ AddIsolatedVertex(m_history, m_da.new_face, GeomPointId{ 9 }) };
  const AddedEdge edge{ AddEdge(m_history, CornerOf(inside), CornerOf(m_a), CurveId{ 9 }) };
  ExpectValid();
  EXPECT_EQ(m_topology.HalfEdgeAt(edge.half_edge).origin, inside);
  EXPECT_TRUE(m_topology.FaceAt(m_da.new_face).inner.empty());
}

TEST_F(HighLevelOpsTest, SplitEdgeThenMergeEdgesRestoresSquare)
{
  MakeSquare();
  const Topology square{ m_topology };
  const VertexId middle{
    SplitEdge(m_history, m_ab, GeomPointId{ 5 }, SplitCurves{ CurveId{ 10 }, CurveId{ 11 } })
  };
  ExpectValid();
  EXPECT_EQ(m_topology.VertexCount(), 5U);
  EXPECT_EQ(m_topology.Target(m_ab), middle);
  MergeEdges(m_history, m_ab, CurveId{ 0 });
  ExpectValid();
  EXPECT_EQ(m_topology, square);
}

TEST_F(HighLevelOpsTest, RemoveEdgeOfFaceMovesItsInnerLoopsOut)
{
  MakeSquare();
  static_cast<void>(AddIsolatedVertex(m_history, m_da.new_face, GeomPointId{ 9 }));
  RemoveEdge(m_history, m_da.half_edge);
  ExpectValid();
  EXPECT_EQ(m_topology.FaceCount(), 1U);
  EXPECT_EQ(m_topology.InnerLoopCount(), 2U);
  m_history.Undo();
  ExpectValid();
  EXPECT_EQ(m_topology.FaceCount(), 2U);
  EXPECT_EQ(m_topology.FaceAt(m_da.new_face).inner.size(), 1U);
}

TEST_F(HighLevelOpsTest, RemoveEdgeOfDanglingEdgeLeavesVertices)
{
  const VertexId a{ AddIsolatedVertex(m_history, m_topology.UnboundedFace(), GeomPointId{ 0 }) };
  const HalfEdgeId edge{ AddDanglingEdge(m_history, CornerOf(a), GeomPointId{ 1 }, CurveId{ 0 }) };
  RemoveEdge(m_history, edge);
  ExpectValid();
  EXPECT_EQ(m_topology.VertexCount(), 2U);
  EXPECT_EQ(m_topology.EdgeCount(), 0U);
}

TEST_F(HighLevelOpsTest, RemoveVertexOfSquareCornerLeavesPath)
{
  MakeSquare();
  RemoveVertex(m_history, m_a);
  ExpectValid();
  EXPECT_EQ(m_topology.VertexCount(), 3U);
  EXPECT_EQ(m_topology.EdgeCount(), 2U);
  EXPECT_EQ(m_topology.FaceCount(), 1U);
  EXPECT_EQ(m_history.UndoCount(), 6U);
}

TEST_F(HighLevelOpsTest, UndoEverythingRestoresEmptyTopology)
{
  MakeSquare();
  RemoveVertex(m_history, m_c);
  while (m_history.UndoCount() > 0)
    m_history.Undo();
  EXPECT_EQ(m_topology, Topology{});
  while (m_history.RedoCount() > 0)
    m_history.Redo();
  ExpectValid();
  EXPECT_EQ(m_topology.VertexCount(), 3U);
}
