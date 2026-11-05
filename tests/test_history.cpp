#include "euler/history.hpp"
#include "topo/topology.hpp"

#include <gtest/gtest.h>
#include <variant>

using jcad::euler::Corner;
using jcad::euler::EulerOp;
using jcad::euler::History;
using jcad::euler::KillEdgeVertex;
using jcad::euler::KillVertexRing;
using jcad::euler::MakeEdgeVertex;
using jcad::euler::MakeVertexRing;
using jcad::topo::GeomPointId;
using jcad::topo::Topology;
using jcad::topo::VertexId;

namespace
{

  class HistoryTest : public ::testing::Test
  {
  protected:
    VertexId AddVertex()
    {
      const EulerOp inverse{ m_history.Apply(
        MakeVertexRing{ m_topology.UnboundedFace(), GeomPointId{ m_point++ }, {}, {} }) };
      return std::get<KillVertexRing>(inverse).vertex;
    }

    void AddDangling(VertexId from)
    {
      m_history.Apply(MakeEdgeVertex{ Corner{ from, m_topology.VertexAt(from).outgoing },
                                      GeomPointId{ m_point++ },
                                      {},
                                      {},
                                      {} });
    }

    Topology m_topology;
    History m_history{ m_topology };
    std::uint32_t m_point{ 0 };
  };

} // namespace

TEST_F(HistoryTest, ApplyOutsideTransactionMakesOneEntryPerOperator)
{
  const VertexId vertex{ AddVertex() };
  AddDangling(vertex);
  EXPECT_EQ(m_history.UndoCount(), 2U);
}

TEST_F(HistoryTest, UndoAndRedoTransactionRestoresStates)
{
  const Topology empty{ m_topology };
  m_history.Begin();
  const VertexId vertex{ AddVertex() };
  AddDangling(vertex);
  AddDangling(vertex);
  m_history.Commit();
  const Topology built{ m_topology };
  ASSERT_EQ(m_history.UndoCount(), 1U);

  m_history.Undo();
  EXPECT_EQ(m_topology, empty);
  EXPECT_EQ(m_history.RedoCount(), 1U);
  m_history.Redo();
  EXPECT_EQ(m_topology, built);
  m_history.Undo();
  m_history.Redo();
  EXPECT_EQ(m_topology, built);
  EXPECT_TRUE(m_topology.Validate().has_value());
}

TEST_F(HistoryTest, CommitNestedTransactionsMakesSingleEntry)
{
  m_history.Begin();
  const VertexId vertex{ AddVertex() };
  m_history.Begin();
  AddDangling(vertex);
  m_history.Commit();
  EXPECT_EQ(m_history.UndoCount(), 0U);
  m_history.Commit();
  EXPECT_EQ(m_history.UndoCount(), 1U);
}

TEST_F(HistoryTest, RollbackUndoesOnlyInnerTransaction)
{
  m_history.Begin();
  const VertexId vertex{ AddVertex() };
  const Topology with_vertex{ m_topology };
  m_history.Begin();
  AddDangling(vertex);
  m_history.Rollback();
  EXPECT_EQ(m_topology, with_vertex);
  m_history.Commit();
  m_history.Undo();
  EXPECT_EQ(m_topology, Topology{});
}

TEST_F(HistoryTest, CommitEmptyTransactionAddsNoEntry)
{
  m_history.Begin();
  m_history.Commit();
  EXPECT_EQ(m_history.UndoCount(), 0U);
}

TEST_F(HistoryTest, CommitAfterUndoClearsRedo)
{
  static_cast<void>(AddVertex());
  m_history.Undo();
  ASSERT_EQ(m_history.RedoCount(), 1U);
  static_cast<void>(AddVertex());
  EXPECT_EQ(m_history.RedoCount(), 0U);
}

TEST_F(HistoryTest, UndoRestoresKilledIdsSoOlderEntriesStayValid)
{
  const VertexId vertex{ AddVertex() };
  AddDangling(vertex);
  const EulerOp kill{ KillEdgeVertex{ m_topology.VertexAt(vertex).outgoing } };
  m_history.Apply(kill);
  m_history.Undo();
  m_history.Undo();
  m_history.Undo();
  EXPECT_EQ(m_topology, Topology{});
  m_history.Redo();
  m_history.Redo();
  m_history.Redo();
  EXPECT_EQ(m_topology.VertexCount(), 1U);
  EXPECT_TRUE(m_topology.Validate().has_value());
}
